
#include "headers.h"
#include "v4l2_names.h"

void uvc_apply_format(struct processing *processing)
{
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct events *events = &processing->events;
    struct v4l2_format fmt;

    if (processing->source.type == ENDPOINT_V4L2 && events->apply_frame_format)
    {
        memset(&fmt, 0, sizeof(struct v4l2_format));
        fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        fmt.fmt.pix.field = V4L2_FIELD_ANY;
        fmt.fmt.pix.width = events->apply_frame_format->wWidth;
        fmt.fmt.pix.height = events->apply_frame_format->wHeight;
        fmt.fmt.pix.pixelformat = events->apply_frame_format->video_format;
        fmt.fmt.pix.sizeimage = fmt.fmt.pix.width * fmt.fmt.pix.height *
                                ((fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) ? 2 : 1);

        if (ioctl(uvc->fd, VIDIOC_S_FMT, &fmt) < 0)
        {
            printf("UVC: Unable to set format %s (%d).\n", strerror(errno), errno);
            return;
        }

        printf("UVC: Setting format: %c%c%c%c %ux%u\n", pixfmtstr(fmt.fmt.pix.pixelformat),
               fmt.fmt.pix.width, fmt.fmt.pix.height);

        memset(&fmt, 0, sizeof(struct v4l2_format));
        fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

        if (ioctl(uvc->fd, VIDIOC_G_FMT, &fmt) < 0)
        {
            printf("UVC: Unable to get format %s (%d).\n", strerror(errno), errno);
            return;
        }

        printf("UVC: Getting format: %c%c%c%c %ux%u\n", pixfmtstr(fmt.fmt.pix.pixelformat),
               fmt.fmt.pix.width, fmt.fmt.pix.height);
    }
}

void uvc_stream_on(struct processing *processing)
{
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct settings *settings = &processing->settings;
    struct v4l2_requestbuffers req;
    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    unsigned int i = 0;
    int payload_size = 0;

    if (processing->target.type == ENDPOINT_UVC &&
        uvc->is_streaming == false)
    {
        printf("UVC: Stream ON init\n");

        uvc->dqbuf_count = 0;
        uvc->qbuf_count = 0;

        memset(&req, 0, sizeof(struct v4l2_requestbuffers));
        req.count = uvc->nbufs;
        req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        req.memory = V4L2_MEMORY_USERPTR;

        if (ioctl(uvc->fd, VIDIOC_REQBUFS, &req) < 0)
        {
            printf("UVC: VIDIOC_REQBUFS error: %s (%d).\n", strerror(errno), errno);
            return;
        }

        if (settings->uvc_buffer_required)
        {
            uvc->dummy_buf = calloc(req.count, sizeof uvc->dummy_buf[0]);
            if (!uvc->dummy_buf)
            {
                printf("UVC: Out of memory\n");
                return;
            }

            payload_size = settings->uvc_buffer_size;

            for (i = 0; i < req.count; ++i)
            {
                uvc->dummy_buf[i].length = payload_size;
                uvc->dummy_buf[i].start = malloc(payload_size);
                if (!uvc->dummy_buf[i].start)
                {
                    printf("UVC: Out of memory\n");
                    return;
                }
            }

            uvc->mem = uvc->dummy_buf;

            for (i = 0; i < uvc->nbufs; ++i)
            {
                struct v4l2_buffer buf;
                memset(&buf, 0, sizeof(struct v4l2_buffer));
                buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
                buf.memory = V4L2_MEMORY_USERPTR;
                buf.m.userptr = (unsigned long)uvc->dummy_buf[i].start;
                buf.length = uvc->dummy_buf[i].length;
                buf.index = i;

                if (ioctl(uvc->fd, VIDIOC_QBUF, &buf) < 0)
                {
                    printf("UVC: VIDIOC_QBUF failed : %s (%d).\n", strerror(errno), errno);
                    return;
                }

                uvc->qbuf_count++;
            }
        }

        if (ioctl(uvc->fd, VIDIOC_STREAMON, &type) < 0)
        {
            printf("UVC: Stream ON failed: %s (%d).\n", strerror(errno), errno);
            return;
        }

        uvc->is_streaming = true;
        printf("UVC: Stream ON success\n");
    }
}

void uvc_stream_off(struct processing *processing)
{
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct settings *settings = &processing->settings;
    struct v4l2_requestbuffers req;
    int type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    unsigned int i;

    if (processing->target.type == ENDPOINT_UVC &&
        uvc->is_streaming == true)
    {
        printf("UVC: Stream OFF init\n");

        if (ioctl(uvc->fd, VIDIOC_STREAMOFF, &type) < 0)
        {
            printf("UVC: STREAM OFF failed: %s (%d).\n", strerror(errno), errno);
            return;
        }

        if (settings->uvc_buffer_required && uvc->dummy_buf)
        {
            printf("UVC: Uninit device\n");

            for (i = 0; i < uvc->nbufs; ++i)
            {
                free(uvc->dummy_buf[i].start);
                uvc->dummy_buf[i].start = NULL;
            }
            free(uvc->dummy_buf);
            uvc->dummy_buf = NULL;
        }

        memset(&req, 0, sizeof(struct v4l2_requestbuffers));
        req.count = 0;
        req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        req.memory = V4L2_MEMORY_USERPTR;

        if (ioctl(uvc->fd, VIDIOC_REQBUFS, &req) < 0)
        {
            printf("UVC: VIDIOC_REQBUFS error: %s (%d).\n", strerror(errno), errno);
            return;
        }

        printf("UVC: Stream OFF success\n");
        uvc->is_streaming = false;
    }
}

void uvc_close(struct processing *processing)
{
    struct endpoint_uvc *uvc;

    if (processing->target.type == ENDPOINT_UVC)
    {
        uvc = &processing->target.uvc;
        if (uvc->fd)
        {
            printf("UVC: Closing %s device\n", uvc->device_name);
            close(uvc->fd);
            uvc->fd = -1;
        }
    }
}

void uvc_init(struct processing *processing,
              const char *device_name,
              unsigned int nbufs)
{
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct v4l2_capability cap;

    if (device_name)
    {
        printf("UVC: Opening %s device\n", device_name);

        uvc->device_name = device_name;
        uvc->fd = open(device_name, O_RDWR | O_NONBLOCK, 0);
        if (uvc->fd == -1)
        {
            printf("UVC: Device open failed: %s (%d).\n", strerror(errno), errno);
            return;
        }

        if (ioctl(uvc->fd, VIDIOC_QUERYCAP, &cap) < 0)
        {
            printf("UVC: VIDIOC_QUERYCAP failed: %s (%d).\n", strerror(errno), errno);
            goto err;
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT))
        {
            printf("UVC: %s is no video output device\n", device_name);
            goto err;
        }

        printf("UVC: Device is %s on bus %s\n", cap.card, cap.bus_info);

        print_v4l2_capabilities("UVC", cap.capabilities);

        uvc->nbufs = nbufs;
        processing->target.type = ENDPOINT_UVC;
        processing->target.state = true;
        return;
    }

err:
    uvc_close(processing);
};
