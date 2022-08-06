
#include "headers.h"

void fb_stream_on(struct processing *processing)
{
    struct endpoint_fb *fb = &processing->source.fb;
    if (processing->source.type == ENDPOINT_FB &&
        fb->fd)
    {
        printf("FB: Stream ON init\n");
        fb->memory = mmap(0,
                          fb->mem_size, PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          fb->fd,
                          0);
        if (fb->memory == MAP_FAILED)
        {
            printf("FB: Can't get framebuffer mmap: %s (%d).\n", strerror(errno), errno);
            fb->memory = NULL;
            return;
        }
        printf("FB: Stream ON success\n");
    }
}

void fb_stream_off(struct processing *processing)
{
    struct endpoint_fb *fb = &processing->source.fb;
    if (processing->source.type == ENDPOINT_FB &&
        fb->memory)
    {
        printf("FB: Stream OFF init\n");
        munmap(fb->memory, 0);
        fb->memory = NULL;
        printf("FB: Stream OFF success\n");
    }
}

static void fb_show_info(struct endpoint_fb *fb)
{
    printf("FB: Resolution: %dx%d\n", fb->width, fb->height);
    printf("FB: Bits per pixel: %d\n", fb->bpp);
    printf("FB: Line length: %d\n", fb->line_length);
    printf("FB: Memory size: %d\n", fb->mem_size);
}

static int fb_get_settings(struct endpoint_fb *fb)
{
    struct fb_var_screeninfo fb_info;
    struct fb_fix_screeninfo mode_info;

    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb_info) < 0)
    {
        printf("FB: Can't get framebuffer info: %s (%d).\n", strerror(errno), errno);
        return -EINVAL;
    }

    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &mode_info))
    {
        printf("FB: Can't get framebuffer screen info: %s (%d).\n", strerror(errno), errno);
        return -EINVAL;
    }

    fb->bpp = fb_info.bits_per_pixel;
    fb->width = fb_info.xres;
    fb->height = fb_info.yres;
    fb->mem_size = mode_info.smem_len;
    fb->line_length = mode_info.line_length;
    fb->screen_size = fb_info.xres * fb_info.yres * fb_info.bits_per_pixel / 8;

    fb_show_info(fb);
    return 1;
}

void fb_close(struct processing *processing)
{
    struct endpoint_fb *fb = &processing->source.fb;

    if (processing->source.type == ENDPOINT_FB &&
        fb->fd)
    {
        printf("FB: Closing %s device\n", fb->device_name);
        close(fb->fd);
        fb->fd = -1;
    }
}

void fb_init(struct processing *processing,
             const char *device_name)
{
    struct endpoint_fb *fb = &processing->source.fb;
    struct settings *settings = &processing->settings;

    if (processing->source.type == ENDPOINT_NONE && device_name)
    {
        printf("FB: Opening %s device\n", device_name);

        fb->device_name = device_name;
        fb->fd = open(device_name, O_RDWR);
        if (fb->fd < 0)
        {
            printf("FB: Device open failed: %s (%d).\n", strerror(errno), errno);
            goto err;
        }

        if (fb_get_settings(fb) < 0)
        {
            goto err;
        }

        processing->source.type = ENDPOINT_FB;
        processing->source.state = true;
        settings->uvc_buffer_required = true;
        settings->uvc_buffer_size = fb->width * fb->height * 2;
        return;
    }

err:
    fb_close(processing);
};
