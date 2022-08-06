
#include "headers.h"

#include "uvc_events.h"
#include "processing_actions.h"
#include "stdin_endpoint.h"
#include "data_buffers.h"

static void stdin_uvc_video_process(struct processing *processing)
{
    struct endpoint_stdin *stdin = &processing->source.stdin;
    struct data_buffers *data_buffers = &processing->data_buffers;
    struct data_buffer *data_buffer = data_buffers->buffer_use;
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct settings *settings = &processing->settings;
    struct events *events = &processing->events;
    struct v4l2_buffer uvc_dequeue;
    struct v4l2_buffer uvc_queue;

    if (!uvc->is_streaming || events->stream == STREAM_OFF || !data_buffer->filled)
    {
        return;
    }

    memset(&uvc_dequeue, 0, sizeof(struct v4l2_buffer));
    uvc_dequeue.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    uvc_dequeue.memory = V4L2_MEMORY_USERPTR;
    if (ioctl(uvc->fd, VIDIOC_DQBUF, &uvc_dequeue) < 0)
    {
        printf("UVC: Unable to dequeue buffer: %s (%d).\n", strerror(errno), errno);
        return;
    }

    memset(&uvc_queue, 0, sizeof(struct v4l2_buffer));
    uvc_queue.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    uvc_queue.memory = V4L2_MEMORY_USERPTR;
    uvc_queue.m.userptr = (unsigned long)data_buffer->start;
    uvc_queue.length = data_buffer->length;
    uvc_queue.index = uvc_dequeue.index;
    uvc_queue.bytesused = data_buffer->bytesused;

    if (ioctl(uvc->fd, VIDIOC_QBUF, &uvc_queue) < 0)
    {
        printf("UVC: Unable to queue buffer: %s (%d).\n", strerror(errno), errno);
        return;
    }

    data_buffer->filled = false;
    data_buffers->fill_buffer = true;

    if (settings->show_fps)
    {
        uvc->buffers_processed++;
    }
}

void processing_loop_stdin_uvc(struct processing *processing)
{
    struct settings *settings = &processing->settings;
    struct endpoint_stdin *stdin = &processing->source.stdin;
    struct data_buffers *data_buffers = &processing->data_buffers;
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct events *events = &processing->events;

    int activity;
    struct timeval tv;
    double next_frame_time = 0;
    double now;
    double diff_time;
    long sleep_time;
    fd_set fdsu;

    printf("PROCESSING: STDIN %c%c%c%c -> UVC %s\n",
        pixfmtstr(stdin->stdin_format),
        uvc->device_name
    );

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

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        activity = select(max(uvc->fd, STDIN_FILENO) + 1, NULL, &dfds, &efds, &tv);

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

        if (data_buffers->fill_buffer)
        {
            fill_buffer_from_stdin(processing);
        }

        if (FD_ISSET(uvc->fd, &dfds))
        {
            now = processing_now();
            if (!*(events->stopped) && now >= next_frame_time)
            {
                stdin_uvc_video_process(processing);

                next_frame_time = now + settings->frame_interval;
            }
        }

        processing_internals(processing);
    }
}