
#include "headers.h"
#include "system.h"

static int system_gpio_write(unsigned int type, char pin[], char value[])
{
    FILE *sys_file;
    char path[255];

    strcpy(path, "/sys/class/gpio/");

    switch (type)
    {
    case GPIO_EXPORT:
        strcat(path, "export");
        value = pin;
        break;

    case GPIO_DIRECTION:
        strcat(path, "gpio");
        strcat(path, pin);
        strcat(path, "/direction");
        break;

    case GPIO_VALUE:
        strcat(path, "gpio");
        strcat(path, pin);
        strcat(path, "/value");
        break;
    }

    printf("GPIO WRITE: Path: %s, Value: %s\n", path, value);

    sys_file = fopen(path, "w");
    if (!sys_file)
    {
        printf("GPIO ERROR: File write failed: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    fwrite(value, 1, strlen(value), sys_file);
    fclose(sys_file);

    return 0;
}

static int system_led_write(unsigned int type, char value[])
{
    FILE *sys_file;
    char path[255];

    strcpy(path, "/sys/class/leds/led0/");

    switch (type)
    {
    case LED_TRIGGER:
        strcat(path, "trigger");
        break;

    case LED_BRIGHTNESS:
        strcat(path, "brightness");
        break;
    }

    printf("LED WRITE: Path: %s, Value: %s\n", path, value);

    sys_file = fopen(path, "w");
    if (!sys_file)
    {
        printf("LED ERROR: File write failed: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    fwrite(value, 1, strlen(value), sys_file);
    fclose(sys_file);

    return 0;
}

void system_show_state(struct processing *processing, bool state)
{
    struct settings *settings = &processing->settings;

    char *gpio_value = (state) ? GPIO_VALUE_ON : GPIO_VALUE_OFF;
    char *led_value = (state) ? LED_BRIGHTNESS_HIGH : LED_BRIGHTNESS_LOW;

    if (settings->streaming_status_enabled)
    {
        system_gpio_write(GPIO_VALUE, settings->streaming_status_pin, gpio_value);
    }

    if (settings->streaming_status_onboard_enabled)
    {
        system_led_write(LED_BRIGHTNESS, led_value);
    }
}

void system_streaming_state(struct processing *processing)
{
    struct endpoint_uvc *uvc = &processing->target.uvc;
    system_show_state(processing, uvc->is_streaming);
}

void system_init(struct processing *processing)
{
    struct settings *settings = &processing->settings;

    if (settings->streaming_status_pin)
    {
        if (system_gpio_write(GPIO_EXPORT, settings->streaming_status_pin, NULL) < 0)
        {
            return;
        }

        if (system_gpio_write(GPIO_DIRECTION, settings->streaming_status_pin, GPIO_DIRECTION_OUT) < 0)
        {
            return;
        }

        if (system_gpio_write(GPIO_VALUE, settings->streaming_status_pin, GPIO_VALUE_OFF) < 0)
        {
            return;
        }

        settings->streaming_status_enabled = true;
    }

    if (settings->streaming_status_onboard)
    {
        if (system_led_write(LED_TRIGGER, LED_TRIGGER_NONE) < 0)
        {
            return;
        }

        if (system_led_write(LED_BRIGHTNESS, LED_BRIGHTNESS_LOW) < 0)
        {
            return;
        }
        settings->streaming_status_onboard_enabled = true;
    }
}
