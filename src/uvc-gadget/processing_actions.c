
#include "headers.h"

#include "fb_endpoint.h"
#include "v4l2_endpoint.h"
#include "uvc_endpoint.h"
#include "system.h"

double processing_now()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (double)((tv.tv_sec + (tv.tv_usec * 1e-6)) * 1000);
}

void processing_internals(struct processing *processing)
{
    struct internals *internals = &processing->internals;
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct settings *settings = &processing->settings;
    struct events *events = &processing->events;

    double now = processing_now();

    if (events->shutdown_requested)
    {
        // *(events->terminate) = true;
        events->stream = STREAM_OFF;
        events->shutdown_requested = false;
    }

    if (settings->show_fps && uvc->is_streaming)
    {
        if (now - uvc->last_time_video_process >= 1000)
        {
            printf("FPS: %d\n", uvc->buffers_processed);
            uvc->buffers_processed = 0;
            uvc->last_time_video_process = now;
        }
    }

    if (settings->blink_on_startup > 0)
    {
        if (now - internals->last_time_blink >= 100)
        {
            internals->blink_state = !(internals->blink_state);

            system_show_state(processing, internals->blink_state);

            internals->last_time_blink = now;
            if (!internals->blink_state)
            {
                settings->blink_on_startup -= 1;
            }

            if (settings->blink_on_startup == 0)
            {
                system_streaming_state(processing);
            }
        }
    }

    if (events->apply_frame_format)
    {
        v4l2_apply_format(processing);
        v4l2_fps_set(processing);
        uvc_apply_format(processing);
        events->apply_frame_format = NULL;
    }

    if (events->control && events->apply_control)
    {
        v4l2_set_ctrl(processing);
    }

    switch (events->stream)
    {
    case STREAM_ON:
        v4l2_stream_on(processing);
        fb_stream_on(processing);
        uvc_stream_on(processing);

        system_streaming_state(processing);
        events->shutdown_requested = false;
        events->stream = STREAM_NONE;
        break;

    case STREAM_OFF:
        v4l2_stream_off(processing);
        fb_stream_off(processing);
        uvc_stream_off(processing);

        system_streaming_state(processing);
        events->stream = STREAM_NONE;
        break;

    default:
        break;
    }
}
