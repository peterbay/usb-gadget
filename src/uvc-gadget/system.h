
#ifndef SYSTEM
#define SYSTEM

#include "headers.h"

enum gpio
{
    GPIO_EXPORT = 0,
    GPIO_DIRECTION,
    GPIO_VALUE,
};

#define GPIO_DIRECTION_OUT "out"
#define GPIO_DIRECTION_IN "in"
#define GPIO_DIRECTION_LOW "low"
#define GPIO_DIRECTION_HIGH "high"

#define GPIO_VALUE_OFF "0"
#define GPIO_VALUE_ON "1"

enum leds
{
    LED_TRIGGER = 1,
    LED_BRIGHTNESS,
};

#define LED_TRIGGER_NONE "none"
#define LED_BRIGHTNESS_LOW "0"
#define LED_BRIGHTNESS_HIGH "1"

void system_init(struct processing *processing);

void system_streaming_state(struct processing *processing);

void system_show_state(struct processing *processing, bool state);

#endif // end SYSTEM
