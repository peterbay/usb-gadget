
#include "headers.h"
#include "uvc_names.h"

void uvc_dump_streaming_control(struct uvc_streaming_control *usc)
{
    printf("UVC Streaming control:\n");
    printf("    bmHint: %d\n", usc->bmHint);
    printf("    bFormatIndex: %d\n", usc->bFormatIndex);
    printf("    bFrameIndex: %d\n", usc->bFrameIndex);
    printf("    dwFrameInterval: %d\n", usc->dwFrameInterval);
    printf("    dwMaxVideoFrameSize: %d\n", usc->dwMaxVideoFrameSize);
    printf("    dwMaxPayloadTransferSize: %d\n", usc->dwMaxPayloadTransferSize);
    printf("    bmFramingInfo: %d\n", usc->bmFramingInfo);
    printf("    bPreferedVersion: %d\n", usc->bPreferedVersion);
    printf("    bMinVersion: %d\n", usc->bMinVersion);
    printf("    bMaxVersion: %d\n", usc->bMaxVersion);
}

static void uvc_dump_frame_format(struct frame_format *frame_format,
                                  const char *title)
{
    printf("%s format: %d, frame: %d, resolution: %dx%d, frame_interval: %d,  bitrate: [%d, %d]\n",
           title,
           frame_format->bFormatIndex,
           frame_format->bFrameIndex,
           frame_format->wWidth,
           frame_format->wHeight,
           frame_format->dwDefaultFrameInterval,
           frame_format->dwMinBitRate,
           frame_format->dwMaxBitRate);
}

static unsigned int get_frame_size(int pixelformat,
                                   int width,
                                   int height)
{
    switch (pixelformat)
    {
    case V4L2_PIX_FMT_YUYV:
        return width * height * 2;

    case V4L2_PIX_FMT_MJPEG:
        return width * height;
        break;
    }

    return width * height;
}

static int uvc_get_frame_format(struct processing *processing,
                                struct frame_format **frame_format,
                                unsigned int iFormat,
                                unsigned int iFrame)
{
    struct configfs *configfs = &processing->configfs;
    unsigned int i;

    for (i = 0; i <= configfs->frame_format_size; i++)
    {
        if (configfs->frame_format[i].bFormatIndex == iFormat &&
            configfs->frame_format[i].bFrameIndex == iFrame)
        {
            *frame_format = &configfs->frame_format[i];
            return 0;
        }
    }
    return -1;
}

static int uvc_get_frame_format_index(struct processing *processing,
                                      int format_index,
                                      enum frame_format_getter getter)
{
    struct configfs *configfs = &processing->configfs;

    int index = -1;
    int value;
    unsigned int i;

    for (i = 0; i <= configfs->frame_format_size; i++)
    {
        if (format_index == -1 || format_index == (int)configfs->frame_format[i].bFormatIndex)
        {

            switch (getter)
            {
            case FORMAT_INDEX_MIN:
            case FORMAT_INDEX_MAX:
                value = configfs->frame_format[i].bFormatIndex;
                break;

            case FRAME_INDEX_MIN:
            case FRAME_INDEX_MAX:
                value = configfs->frame_format[i].bFrameIndex;
                break;
            }
            if (index == -1)
            {
                index = value;
            }
            else
            {
                switch (getter)
                {
                case FORMAT_INDEX_MIN:
                case FRAME_INDEX_MIN:
                    if (value < index)
                    {
                        index = value;
                    }
                    break;

                case FORMAT_INDEX_MAX:
                case FRAME_INDEX_MAX:
                    if (value > index)
                    {
                        index = value;
                    }
                    break;
                }
            }
        }
    }
    return index;
}

void uvc_fill_streaming_control(struct processing *processing,
                                struct uvc_streaming_control *ctrl_out,
                                enum stream_control_action action,
                                struct uvc_streaming_control *ctrl_in)
{
    struct configfs *configfs = &processing->configfs;
    struct settings *settings = &processing->settings;
    struct events *events = &processing->events;
    struct uvc_request *uvc_request = &processing->uvc_request;
    struct frame_format *frame_format = NULL;

    int iformat = 0;
    int iframe = 0;
    int format_first;
    int format_last;
    int frame_first;
    int frame_last;
    int format_frame_first;
    int format_frame_last;
    unsigned int frame_interval;
    unsigned int dwMaxPayloadTransferSize;

    switch (action)
    {
    case STREAM_CONTROL_INIT:
        printf("UVC: Streaming control action: INIT\n");
        break;

    case STREAM_CONTROL_MIN:
        printf("UVC: Streaming control action: GET MIN\n");
        break;

    case STREAM_CONTROL_MAX:
        printf("UVC: Streaming control action: GET MAX\n");
        break;

    case STREAM_CONTROL_SET:
        printf("UVC: Streaming control action: SET\n");
        break;
    }

    if (ctrl_in)
    {
        iformat = ctrl_in->bFormatIndex;
        iframe = ctrl_in->bFrameIndex;

        if (action == STREAM_CONTROL_SET)
        {
            printf("UVC: SET format: %d, frame: %d\n", iformat, iframe);
        }

        if (settings->debug)
        {
            printf("UVC: Control IN: ");
            uvc_dump_streaming_control(ctrl_in);
        }
    }

    format_first = uvc_get_frame_format_index(processing, -1, FORMAT_INDEX_MIN);
    format_last = uvc_get_frame_format_index(processing, -1, FORMAT_INDEX_MAX);

    frame_first = uvc_get_frame_format_index(processing, -1, FRAME_INDEX_MIN);
    frame_last = uvc_get_frame_format_index(processing, -1, FRAME_INDEX_MAX);

    if (action == STREAM_CONTROL_MIN)
    {
        iformat = format_first;
        iframe = frame_first;
    }
    else if (action == STREAM_CONTROL_MAX)
    {
        iformat = format_last;
        iframe = frame_last;
    }
    else
    {
        iformat = clamp(iformat, format_first, format_last);

        format_frame_first = uvc_get_frame_format_index(processing, iformat, FRAME_INDEX_MIN);
        format_frame_last = uvc_get_frame_format_index(processing, iformat, FRAME_INDEX_MAX);

        iframe = clamp(iframe, format_frame_first, format_frame_last);
    }

    uvc_get_frame_format(processing, &frame_format, iformat, iframe);

    frame_interval = frame_format->dwDefaultFrameInterval;

    dwMaxPayloadTransferSize = configfs->streaming.maxpacket;
    if (configfs->streaming.maxpacket > 1024 && configfs->streaming.maxpacket % 1024 != 0)
    {
        dwMaxPayloadTransferSize -= (configfs->streaming.maxpacket / 1024) * 128;
    }

    memset(ctrl_out, 0, sizeof *ctrl_out);
    ctrl_out->bmHint = 1;
    ctrl_out->bFormatIndex = iformat;
    ctrl_out->bFrameIndex = iframe;
    ctrl_out->dwFrameInterval = frame_interval;
    ctrl_out->wKeyFrameRate = 0;
    ctrl_out->wPFrameRate = 0;
    ctrl_out->wCompQuality = 0;
    ctrl_out->wCompWindowSize = 0;
    ctrl_out->wDelay = 0;
    ctrl_out->dwMaxVideoFrameSize = get_frame_size(frame_format->video_format, frame_format->wWidth, frame_format->wHeight);
    ctrl_out->dwMaxPayloadTransferSize = dwMaxPayloadTransferSize;
    ctrl_out->dwClockFrequency = 0;
    ctrl_out->bmFramingInfo = 3;
    ctrl_out->bPreferedVersion = format_last;
    ctrl_out->bMinVersion = format_first;
    ctrl_out->bMaxVersion = format_last;

    if (settings->debug)
    {
        printf("UVC: Control OUT: ");
        uvc_dump_streaming_control(ctrl_out);
    }

    if (uvc_request->control == UVC_VS_COMMIT_CONTROL && action == STREAM_CONTROL_SET)
    {
        if (settings->debug)
        {
            uvc_dump_frame_format(frame_format, "UVC: Frame");
        }

        events->apply_frame_format = frame_format;
    }
}
