
#include "headers.h"

char *paths_configfs[3000];
static unsigned int index_configfs = 0;

static void dump_frame_format(struct frame_format *frame_format,
                              const char *title)
{
    printf("%s format: %d, frame: %d, resolution: %dx%d, interval: %d, bitrate: [%d, %d]\n",
           title,
           frame_format->bFormatIndex,
           frame_format->bFrameIndex,
           frame_format->wWidth,
           frame_format->wHeight,
           frame_format->dwDefaultFrameInterval,
           frame_format->dwMinBitRate,
           frame_format->dwMaxBitRate);
}

static int find_text_pos(const char *s,
                         const char *f)
{
    char *p = strstr(s, f);
    return (p) ? p - s : 0;
}

static int configfs_read_value(const char *path)
{
    char buf[20];
    int fd;
    int ret;

    fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        return -ENOENT;
    }
    ret = read(fd, buf, 20);
    close(fd);

    if (ret < 0 || ret > 10)
    {
        return -ENODATA;
    }
    buf[ret] = '\0';
    return strtol(buf, NULL, 10);
}

static void set_uvc_format_index(struct configfs *configfs,
                                 enum usb_device_speed usb_speed,
                                 int video_format,
                                 unsigned int bFormatIndex)
{
    unsigned int i;
    for (i = 0; i <= configfs->frame_format_size; i++)
    {
        if (configfs->frame_format[i].usb_speed == usb_speed &&
            configfs->frame_format[i].video_format == video_format)
        {
            configfs->frame_format[i].bFormatIndex = bFormatIndex;
        }
    }
}

static void set_format_value(struct frame_format *format,
                             const char *key_word,
                             int value)
{
    if (!strncmp(key_word, "dwDefaultFrameInterval", 22))
    {
        format->dwDefaultFrameInterval = value;
    }
    else if (!strncmp(key_word, "dwMaxVideoFrameBufferSize", 25))
    {
        format->dwMaxVideoFrameBufferSize = value;
    }
    else if (!strncmp(key_word, "dwMaxBitRate", 12))
    {
        format->dwMaxBitRate = value;
    }
    else if (!strncmp(key_word, "dwMinBitRate", 12))
    {
        format->dwMinBitRate = value;
    }
    else if (!strncmp(key_word, "wHeight", 7))
    {
        format->wHeight = value;
    }
    else if (!strncmp(key_word, "wWidth", 6))
    {
        format->wWidth = value;
    }
    else if (!strncmp(key_word, "bmCapabilities", 14))
    {
        format->bmCapabilities = value;
    }
    else if (!strncmp(key_word, "bFrameIndex", 11))
    {
        format->bFrameIndex = value;
    }
}

static int configfs_usb_speed(const char *speed)
{
    if (!strncmp(speed, "fs", 2))
    {
        return USB_SPEED_FULL;
    }
    else if (!strncmp(speed, "hs", 2))
    {
        return USB_SPEED_HIGH;
    }
    else if (!strncmp(speed, "ss", 2))
    {
        return USB_SPEED_SUPER;
    }
    return USB_SPEED_UNKNOWN;
}

static int configfs_video_format(const char *format)
{
    if (!strncmp(format, "m", 1))
    {
        return V4L2_PIX_FMT_MJPEG;
    }
    else if (!strncmp(format, "u", 1))
    {
        return V4L2_PIX_FMT_YUYV;
    }
    return 0;
}

static void configfs_fill_formats(struct configfs *configfs,
                                  const char *path,
                                  const char *part)
{
    const char *format_name;
    enum usb_device_speed usb_speed;
    char *array[10];
    char *copy = strdup(part);
    char *token = strtok(copy, "/");
    int index = 0;
    int value = 0;
    int video_format;

    while (token != NULL)
    {
        array[index++] = token;
        token = strtok(NULL, "/");
    }

    if (index > 3)
    {
        format_name = array[3];

        usb_speed = configfs_usb_speed(array[0]);
        if (usb_speed == USB_SPEED_UNKNOWN)
        {
            printf("CONFIGFS: Unsupported USB speed: (%s) %s\n", array[0], path);
            goto free;
        }

        video_format = configfs_video_format(array[2]);
        if (video_format == 0)
        {
            printf("CONFIGFS: Unsupported format: (%s) %s\n", array[2], path);
            goto free;
        }

        value = configfs_read_value(path);
        if (value < 0)
        {
            goto free;
        }

        if (!strncmp(array[index - 1], "bFormatIndex", 12))
        {
            set_uvc_format_index(configfs, usb_speed, video_format, value);
            goto free;
        }

        if (index != 5)
        {
            goto free;
        }

        if (
            configfs->frame_format[configfs->frame_format_size].usb_speed != usb_speed ||
            configfs->frame_format[configfs->frame_format_size].video_format != video_format ||
            strncmp(configfs->frame_format[configfs->frame_format_size].format_name, format_name, strlen(format_name)))
        {
            if (configfs->frame_format[configfs->frame_format_size].defined)
            {
                configfs->frame_format_size += 1;

                /* too much defined formats */
                if (configfs->frame_format_size > 29)
                {
                    goto free;
                }
            }

            configfs->frame_format[configfs->frame_format_size].usb_speed = usb_speed;
            configfs->frame_format[configfs->frame_format_size].video_format = video_format;
            configfs->frame_format[configfs->frame_format_size].format_name = strdup(format_name);
            configfs->frame_format[configfs->frame_format_size].defined = true;
        }

        set_format_value(&configfs->frame_format[configfs->frame_format_size], array[index - 1], value);
    }

free:
    free(copy);
}

static void configfs_fill_streaming_params(struct configfs *configfs,
                                           const char *path,
                                           const char *part)
{
    int value = configfs_read_value(path);

    if (!strncmp(part, "maxburst", 8))
    {
        configfs->streaming.maxburst = clamp(value, 0, 15);
    }
    else if (!strncmp(part, "maxpacket", 9))
    {
        configfs->streaming.maxpacket = clamp(value, 1, 3072);
    }
    else if (!strncmp(part, "interval", 8))
    {
        configfs->streaming.interval = clamp(value, 1, 16);
    }
}

static int configfs_read(const char *fpath,
                         const struct stat *sb,
                         int tflag)
{
    int uvc = find_text_pos(fpath, "/uvc");
    (void)(tflag); /* avoid warning: unused parameter 'tflag' */

    if (!S_ISDIR(sb->st_mode) && uvc && index_configfs < 2999)
    {
        paths_configfs[index_configfs] = strdup(fpath);
        index_configfs += 1;
    }
    return 0;
}

static char *ltrim(char *s)
{
    while (isspace(*s))
    {
        s++;
    }
    return s;
}

static char *rtrim(char *s)
{
    char *back = s + strlen(s);
    while (isspace(*--back))
        ;
    *(back + 1) = '\0';
    return s;
}

static char *trim(char *s)
{
    return rtrim(ltrim(s));
}

void configfs_get_mountpoint(struct processing *processing)
{
    struct configfs *configfs = &processing->configfs;

    FILE *fp;
    size_t len = 0;
    char *line = NULL;
    ssize_t read;
    char *ptr;
    char *entry;

    configfs->mount_point_found = false;

    fp = fopen("/proc/mounts", "r");
    if (fp)
    {
        printf("CONFIGFS: Open /proc/mounts\n");

        while ((read = getline(&line, &len, fp)) != -1)
        {
            ptr = strtok(trim(line), " \t");
            if (ptr == NULL)
            {
                continue;
            }

            entry = trim(ptr);

            if (!strncmp(entry, "configfs", 8))
            {
                ptr = strtok(NULL, " \t\n");
                if (ptr != NULL)
                {
                    entry = trim(ptr);
                    strcpy(configfs->mount_point, ptr);
                    strcat(configfs->mount_point, "/usb_gadget");

                    configfs->mount_point_found = true;
                    break;
                }
            }
        }
        fclose(fp);
        printf("CONFIGFS: Close /proc/mounts\n");
    }
    else
    {
        printf("CONFIGFS: ERROR: Can't open /proc/mounts\n");
    }
}

int configfs_init(struct processing *processing)
{
    struct configfs *configfs = &processing->configfs;

    const char *fpath;
    int streaming_params;
    int streaming;
    unsigned int i;

    configfs->frame_format_size = 0;
    configfs->streaming.maxburst = 0;
    configfs->streaming.maxpacket = 1023;
    configfs->streaming.interval = 1;

    printf("CONFIGFS: Getting mountpoint for configfs from /proc/mounts\n");

    configfs_get_mountpoint(processing);

    if (!configfs->mount_point_found)
    {
        printf("CONFIGFS: ERROR: Missing configfs mountpoint\n");
        return -1;
    }

    printf("CONFIGFS: Initial path: %s\n", configfs->mount_point);

    if (ftw(configfs->mount_point, configfs_read, 20) == -1)
    {
        return -1;
    }

    for (i = 0; i < index_configfs; i++)
    {
        fpath = paths_configfs[i];
        streaming = find_text_pos(fpath, "streaming/class/");
        streaming_params = find_text_pos(fpath, "/streaming_");

        if (streaming)
        {
            configfs_fill_formats(configfs, fpath, fpath + streaming + 16);
        }
        else if (streaming_params)
        {
            configfs_fill_streaming_params(configfs, fpath, fpath + streaming_params + 11);
        }
        free(paths_configfs[i]);
        paths_configfs[i] = NULL;
    }
    index_configfs = 0;

    if (!configfs->frame_format[0].defined)
    {
        return -1;
    }

    for (i = 0; i <= configfs->frame_format_size; i++)
    {
        dump_frame_format(&configfs->frame_format[i], "CONFIGFS: UVC");
    }

    printf("CONFIGFS: STREAMING maxburst:  %d\n", configfs->streaming.maxburst);
    printf("CONFIGFS: STREAMING maxpacket: %d\n", configfs->streaming.maxpacket);
    printf("CONFIGFS: STREAMING interval:  %d\n", configfs->streaming.interval);

    return 1;
}