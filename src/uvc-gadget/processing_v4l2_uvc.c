
#include "headers.h"
#include "uvc_events.h"
#include "processing_actions.h"

static bool v4l2_buffer_flags_print(bool show_header, const char * name)
{
    printf("%s%s", (show_header) ? "V4L2 buffer flags: " : ", ", name);
    return false;
}

static void v4l2_buffer_flags(struct v4l2_buffer buffer)
{
    bool show_header = true;
    if (buffer.flags & V4L2_BUF_FLAG_MAPPED) {
        show_header = v4l2_buffer_flags_print(show_header, "MAPPED");
    }

    if (buffer.flags & V4L2_BUF_FLAG_QUEUED) {
        show_header = v4l2_buffer_flags_print(show_header, "QUEUED");
    }

    if (buffer.flags & V4L2_BUF_FLAG_DONE) {
        show_header = v4l2_buffer_flags_print(show_header, "DONE");
    }

    if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
        show_header = v4l2_buffer_flags_print(show_header, "ERROR");
    }

    if (buffer.flags & V4L2_BUF_FLAG_IN_REQUEST) {
        show_header = v4l2_buffer_flags_print(show_header, "IN_REQUEST");
    }

    if (buffer.flags & V4L2_BUF_FLAG_KEYFRAME) {
        show_header = v4l2_buffer_flags_print(show_header, "KEYFRAME");
    }

    if (buffer.flags & V4L2_BUF_FLAG_PFRAME) {
        show_header = v4l2_buffer_flags_print(show_header, "PFRAME");
    }

    if (buffer.flags & V4L2_BUF_FLAG_BFRAME) {
        show_header = v4l2_buffer_flags_print(show_header, "BFRAME");
    }

    if (buffer.flags & V4L2_BUF_FLAG_TIMECODE) {
        show_header = v4l2_buffer_flags_print(show_header, "TIMECODE");
    }

    if (buffer.flags & V4L2_BUF_FLAG_PREPARED) {
        show_header = v4l2_buffer_flags_print(show_header, "PREPARED");
    }

    if (buffer.flags & V4L2_BUF_FLAG_NO_CACHE_INVALIDATE) {
        show_header = v4l2_buffer_flags_print(show_header, "NO_CACHE_INVALIDATE");
    }

    if (buffer.flags & V4L2_BUF_FLAG_NO_CACHE_CLEAN) {
        show_header = v4l2_buffer_flags_print(show_header, "NO_CACHE_CLEAN");
    }

    if (buffer.flags & V4L2_BUF_FLAG_M2M_HOLD_CAPTURE_BUF) {
        show_header = v4l2_buffer_flags_print(show_header, "M2M_HOLD_CAPTURE_BUF");
    }

    if (buffer.flags & V4L2_BUF_FLAG_REQUEST_FD) {
        show_header = v4l2_buffer_flags_print(show_header, "REQUEST_FD");
    }

    if (buffer.flags & V4L2_BUF_FLAG_TIMESTAMP_MASK) {
        show_header = v4l2_buffer_flags_print(show_header, "TIMESTAMP_MASK");
    }

    if (buffer.flags & V4L2_BUF_FLAG_TIMESTAMP_UNKNOWN) {
        show_header = v4l2_buffer_flags_print(show_header, "TIMESTAMP_UNKNOWN");
    }

    if (buffer.flags & V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC) {
        show_header = v4l2_buffer_flags_print(show_header, "TIMESTAMP_MONOTONIC");
    }

    if (buffer.flags & V4L2_BUF_FLAG_TIMESTAMP_COPY) {
        show_header = v4l2_buffer_flags_print(show_header, "TIMESTAMP_COPY");
    }

    if (buffer.flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK) {
        show_header = v4l2_buffer_flags_print(show_header, "TSTAMP_SRC_MASK");
    }

    if (buffer.flags & V4L2_BUF_FLAG_TSTAMP_SRC_EOF) {
        show_header = v4l2_buffer_flags_print(show_header, "TSTAMP_SRC_EOF");
    }

    if (buffer.flags & V4L2_BUF_FLAG_TSTAMP_SRC_SOE) {
        show_header = v4l2_buffer_flags_print(show_header, "TSTAMP_SRC_SOE");
    }

    if (!show_header) {
        printf("\n");
    }
}

static void uvc_v4l2_video_process(struct processing *processing)
{
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct events *events = &processing->events;
    struct settings *settings = &processing->settings;
    struct v4l2_buffer ubuf;
    struct v4l2_buffer vbuf;

    if ((!events->shutdown_requested && ((uvc->dqbuf_count + 1) >= uvc->qbuf_count)) ||
        events->stream == STREAM_OFF)
    {
        return;
    }

    memset(&ubuf, 0, sizeof(struct v4l2_buffer));
    ubuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ubuf.memory = V4L2_MEMORY_USERPTR;
    if (ioctl(uvc->fd, VIDIOC_DQBUF, &ubuf) < 0)
    {
        printf("UVC: Unable to dequeue buffer: %s (%d).\n", strerror(errno), errno);
        return;
    }

    uvc->dqbuf_count++;

    if (ubuf.flags & V4L2_BUF_FLAG_ERROR)
    {
        // events->shutdown_requested = true;
        printf("UVC: Possible USB shutdown requested from Host, seen during VIDIOC_DQBUF\n");
        // return;
    }

    v4l2_buffer_flags(ubuf);
    printf("buffer used %d\n", ubuf.bytesused);

    memset(&vbuf, 0, sizeof(struct v4l2_buffer));
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;
    vbuf.index = ubuf.index;
    if (ioctl(v4l2->fd, VIDIOC_QBUF, &vbuf) < 0)
    {
        printf("V4L2: Unable to queue buffer: %s (%d).\n", strerror(errno), errno);
        return;
    }

    v4l2->qbuf_count++;

    if (settings->show_fps)
    {
        uvc->buffers_processed++;
    }
}

static void v4l2_uvc_video_process(struct processing *processing)
{
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct events *events = &processing->events;
    struct settings *settings = &processing->settings;
    struct v4l2_buffer vbuf;
    struct v4l2_buffer ubuf;

    if (uvc->is_streaming && v4l2->dqbuf_count >= v4l2->qbuf_count)
    {
        return;
    }

    memset(&vbuf, 0, sizeof(struct v4l2_buffer));
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(v4l2->fd, VIDIOC_DQBUF, &vbuf) < 0)
    {
        printf("V4L2: Unable to dequeue buffer: %s (%d).\n", strerror(errno), errno);
        return;
    }

    v4l2->dqbuf_count++;

    memset(&ubuf, 0, sizeof(struct v4l2_buffer));
    ubuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ubuf.memory = V4L2_MEMORY_USERPTR;
    ubuf.m.userptr = (unsigned long)v4l2->mem[vbuf.index].start;
    ubuf.length = v4l2->mem[vbuf.index].length;
    ubuf.index = vbuf.index;
    ubuf.bytesused = vbuf.bytesused;
    if (ioctl(uvc->fd, VIDIOC_QBUF, &ubuf) < 0)
    {
        if (errno == ENODEV)
        {
            events->shutdown_requested = true;
            printf("UVC: Possible USB shutdown requested from Host, seen during VIDIOC_QBUF\n");
        }
        return;
    }

    uvc->qbuf_count++;

    if (!uvc->is_streaming)
    {
        processing->events.stream = STREAM_ON;
        settings->blink_on_startup = 0;
    }
}

void processing_loop_v4l2_uvc(struct processing *processing)
{
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct events *events = &processing->events;
    struct timeval tv;
    int activity;
    fd_set fdsv, fdsu;

    printf("PROCESSING: V4L2 %s -> UVC %s\n", v4l2->device_name, uvc->device_name);

    while (!*(events->terminate))
    {
        FD_ZERO(&fdsv);
        FD_ZERO(&fdsu);

        FD_SET(uvc->fd, &fdsu);
        FD_SET(v4l2->fd, &fdsv);

        fd_set efds = fdsu;
        fd_set dfds = fdsu;

        // nanosleep((const struct timespec[]){{0, 1000000L}}, NULL);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        if (v4l2->is_streaming)
        {
            activity = select(max(v4l2->fd, uvc->fd) + 1, &fdsv, &dfds, &efds, &tv);

            if (activity == 0)
            {
                printf("PROCESSING: Select timeout\n");
                break;
            }
        }
        else
        {
            activity = select(uvc->fd + 1, NULL, &dfds, &efds, NULL);
        }

        if (activity == -1)
        {
            printf("PROCESSING: Select error %d, %s\n", errno, strerror(errno));
            if (errno == EINTR)
            {
                continue;
            }
            break;
        }

        if (FD_ISSET(uvc->fd, &efds))
        {
            uvc_events_process(processing);
        }

        if (v4l2->is_streaming && !*(events->stopped))
        {
            if (FD_ISSET(uvc->fd, &dfds))
            {
                uvc_v4l2_video_process(processing);
            }

            if (FD_ISSET(v4l2->fd, &fdsv))
            {
                v4l2_uvc_video_process(processing);
            }
        }

        processing_internals(processing);
    }
}
