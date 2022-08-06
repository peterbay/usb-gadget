
#include "headers.h"

#include "uvc_events.h"
#include "rgb_yuv.h"
#include "processing_actions.h"

static void fb_fill_uvc_buffer(struct processing *processing,
                               struct v4l2_buffer *uvc_bufer)
{
    struct endpoint_fb *fb = &processing->source.fb;
    struct endpoint_uvc *uvc = &processing->target.uvc;

    unsigned int yvyu = 0;
    unsigned char r1;
    unsigned char b1;
    unsigned char g1;
    unsigned char r2;
    unsigned char b2;
    unsigned char g2;
    unsigned char r12;
    unsigned char g12;
    unsigned char b12;
    unsigned int size = fb->height * fb->width;
    char *uvc_pixels = (char *)uvc->mem[uvc_bufer->index].start;
    char *fb_pixels = (char *)fb->memory;
    uvc_bufer->bytesused = size * 2;

    struct rgb_pixel24 rgb_pixel24;
    struct rgb_pixel32 rgb_pixel32;
    struct yuyv_pixel yuyv_pixel;

    memset(&rgb_pixel24, 0, sizeof(struct rgb_pixel24));
    memset(&rgb_pixel32, 0, sizeof(struct rgb_pixel32));
    memset(&yuyv_pixel, 0, sizeof(unsigned int));

    switch (fb->bpp)
    {
    case 16:
        while (size)
        {
            b1 = (*(fb_pixels)&0x1f) << 3;
            g1 = (((*(fb_pixels + 1) & 0x7) << 3) | (*(fb_pixels)&0xE0) >> 5) << 2;
            r1 = (*(fb_pixels + 1) & 0xF8);
            b2 = (*(fb_pixels + 2) & 0x1f) << 3;
            g2 = (((*(fb_pixels + 3) & 0x7) << 3) | (*(fb_pixels + 2) & 0xE0) >> 5) << 2;
            r2 = (*(fb_pixels + 3) & 0xF8);
            yvyu = rgb2yvyu(r1, g1, b1, r2, g2, b2);
            memcpy(uvc_pixels, &yvyu, 4);
            fb_pixels += 4;
            uvc_pixels += 4;
            size -= 2;
        }
        break;

    case 24:
        while (size)
        {
            memcpy(&rgb_pixel24.rgba1, fb_pixels, 3);
            memcpy(&rgb_pixel24.rgba2, fb_pixels + 3, 3);

            r12 = rgb_pixel24.x1.r + rgb_pixel24.x2.r;
            g12 = rgb_pixel24.x1.g + rgb_pixel24.x2.g;
            b12 = rgb_pixel24.x1.b + rgb_pixel24.x2.b;

            yuyv_pixel.z.y1 = rgb_pixel24.y1.r + rgb_pixel24.y1.g + rgb_pixel24.y1.b + 16;
            yuyv_pixel.z.u = mult_112[r12] - mult_94[g12] - mult_18[b12];
            yuyv_pixel.z.y2 = rgb_pixel24.y2.r + rgb_pixel24.y2.g + rgb_pixel24.y2.b + 16;
            yuyv_pixel.z.v = mult_112[b12] - mult_38[r12] - mult_74[g12];

            memcpy(uvc_pixels, &yuyv_pixel.yuyv, 4);

            fb_pixels += 6;
            uvc_pixels += 4;
            size -= 2;
        }
        break;

    case 32:
        while (size)
        {
            memcpy(&rgb_pixel32, fb_pixels, 8);

            r12 = rgb_pixel32.x1.r + rgb_pixel32.x2.r;
            g12 = rgb_pixel32.x1.g + rgb_pixel32.x2.g;
            b12 = rgb_pixel32.x1.b + rgb_pixel32.x2.b;

            yuyv_pixel.z.y1 = rgb_pixel32.y1.r + rgb_pixel32.y1.g + rgb_pixel32.y1.b + 16;
            yuyv_pixel.z.u = mult_112[r12] - mult_94[g12] - mult_18[b12];
            yuyv_pixel.z.y2 = rgb_pixel32.y2.r + rgb_pixel32.y2.g + rgb_pixel32.y2.b + 16;
            yuyv_pixel.z.v = mult_112[b12] - mult_38[r12] - mult_74[g12];

            memcpy(uvc_pixels, &yuyv_pixel.yuyv, 4);

            fb_pixels += 8;
            uvc_pixels += 4;
            size -= 2;
        }
        break;
    }
}

static void fb_uvc_video_process(struct processing *processing)
{
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct settings *settings = &processing->settings;
    struct events *events = &processing->events;
    struct v4l2_buffer uvc_buffer;

    if (!uvc->is_streaming || events->stream == STREAM_OFF)
    {
        return;
    }

    memset(&uvc_buffer, 0, sizeof(struct v4l2_buffer));
    uvc_buffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    uvc_buffer.memory = V4L2_MEMORY_USERPTR;
    if (ioctl(uvc->fd, VIDIOC_DQBUF, &uvc_buffer) < 0)
    {
        printf("UVC: Unable to dequeue buffer: %s (%d).\n", strerror(errno), errno);
        return;
    }

    fb_fill_uvc_buffer(processing, &uvc_buffer);

    if (ioctl(uvc->fd, VIDIOC_QBUF, &uvc_buffer) < 0)
    {
        printf("UVC: Unable to queue buffer: %s (%d).\n", strerror(errno), errno);
        return;
    }

    if (settings->show_fps)
    {
        uvc->buffers_processed++;
    }
}

void processing_loop_fb_uvc(struct processing *processing)
{
    struct endpoint_fb *fb = &processing->source.fb;
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct settings *settings = &processing->settings;
    struct events *events = &processing->events;

    int activity;
    double next_frame_time = 0;
    double now;
    double diff_time;
    long sleep_time;
    fd_set fdsu;

    printf("PROCESSING: FB %s -> UVC %s\n", fb->device_name, uvc->device_name);

    while (!*(events->terminate))
    {
        FD_ZERO(&fdsu);
        FD_SET(uvc->fd, &fdsu);

        fd_set efds = fdsu;
        fd_set dfds = fdsu;

        now = processing_now();
        diff_time = (next_frame_time - now);

        if (diff_time > 0)
        {
            sleep_time = (long)(diff_time * 0.8);
            if (sleep_time > 5)
            {
                sleep_time = 5;
            }
            nanosleep((const struct timespec[]){{0, sleep_time * 1000000L}}, NULL);
        }

        activity = select(uvc->fd + 1, NULL, &dfds, &efds, NULL);

        if (activity == -1)
        {
            printf("PROCESSING: Select error %d, %s\n", errno, strerror(errno));
            if (errno == EINTR)
            {
                continue;
            }
            break;
        }

        if (activity == 0)
        {
            printf("PROCESSING: Select timeout\n");
            break;
        }

        if (FD_ISSET(uvc->fd, &efds))
        {
            uvc_events_process(processing);
        }

        if (FD_ISSET(uvc->fd, &dfds))
        {
            now = processing_now();
            if (!*(events->stopped) && now >= next_frame_time && fb->memory)
            {
                fb_uvc_video_process(processing);
                next_frame_time = now + settings->frame_interval;
            }
        }

        processing_internals(processing);
    }
}