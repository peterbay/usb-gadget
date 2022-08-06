
#include "headers.h"

#include "uvc_names.h"
#include "uvc_control.h"

static void uvc_interface_control(struct processing *processing)
{
    struct controls *controls = &processing->controls;
    struct uvc_request *uvc_request = &processing->uvc_request;
    struct events *events = &processing->events;

    int i;
    const char *request_code_name = uvc_request_code_name(uvc_request->request);
    const char *interface_name;
    const char *control_name = "UNKNOWN";
    struct control_mapping_pair *mapping = NULL;
    uint8_t interface;

    switch (uvc_request->interface)
    {
    case 0:
        if (uvc_request->control == UVC_VC_REQUEST_ERROR_CODE_CONTROL)
        {
            uvc_request->response.data[0] = uvc_request->response_code;
            uvc_request->response.length = 1;
        }
        return;

    case 1:
        interface_name = "INPUT_TERMINAL";
        interface = UVC_VC_INPUT_TERMINAL;
        control_name = uvc_terminal_control_name(uvc_request->control);
        break;

    case 2:
        interface_name = "PROCESSING_UNIT";
        interface = UVC_VC_PROCESSING_UNIT;
        control_name = uvc_processing_control_name(uvc_request->control);
        break;

    default:
        return;
    }

    for (i = 0; i < controls->size; i++)
    {
        if (controls->mapping[i].type == interface &&
            controls->mapping[i].uvc == uvc_request->control)
        {
            mapping = &controls->mapping[i];
            break;
        }
    }

    if (!mapping)
    {
        printf("UVC: Request interface: %s, type: %s, control: %s - UNSUPPORTED\n",
               interface_name, request_code_name, control_name);
        uvc_request->response.length = -EL2HLT;
        uvc_request->response_code = REQEC_INVALID_CONTROL;
        return;
    }

    if (!mapping->enabled)
    {
        printf("UVC: Request interface: %s, type: %s, control: %s - DISABLED\n",
               interface_name, request_code_name, control_name);
        uvc_request->response.length = -EL2HLT;
        uvc_request->response_code = REQEC_INVALID_CONTROL;
        return;
    }

    printf("UVC: Request type: %s, control: %s\n", request_code_name, control_name);

    uvc_request->response_code = REQEC_NO_ERROR;

    switch (uvc_request->request)
    {
    case UVC_SET_CUR:
        uvc_request->response.data[0] = 0x0;
        uvc_request->response.length = uvc_request->length;
        events->control = mapping;
        break;

    case UVC_GET_MIN:
        uvc_request->response.length = 4;
        memcpy(&uvc_request->response.data[0], &mapping->minimum, uvc_request->response.length);
        break;

    case UVC_GET_MAX:
        uvc_request->response.length = 4;
        memcpy(&uvc_request->response.data[0], &mapping->maximum, uvc_request->response.length);
        break;

    case UVC_GET_CUR:
        uvc_request->response.length = 4;
        memcpy(&uvc_request->response.data[0], &mapping->value, uvc_request->response.length);
        break;

    case UVC_GET_INFO:
        uvc_request->response.length = 1;
        uvc_request->response.data[0] = (uint8_t)(UVC_CONTROL_CAP_GET | UVC_CONTROL_CAP_SET);
        break;

    case UVC_GET_DEF:
        uvc_request->response.length = 4;
        memcpy(&uvc_request->response.data[0], &mapping->default_value, uvc_request->response.length);
        break;

    case UVC_GET_RES:
        uvc_request->response.length = 4;
        memcpy(&uvc_request->response.data[0], &mapping->step, uvc_request->response.length);
        break;

    default:
        uvc_request->response.length = -EL2HLT;
        uvc_request->response_code = REQEC_INVALID_REQUEST;
        break;
    }
    return;
}

static void uvc_events_process_streaming(struct processing *processing)
{
    struct uvc_request *uvc_request = &processing->uvc_request;
    struct endpoint_uvc *uvc = &processing->target.uvc;

    if (uvc_request->control != UVC_VS_PROBE_CONTROL &&
        uvc_request->control != UVC_VS_COMMIT_CONTROL)
    {
        return;
    }

    printf("UVC: Streaming request control: %s, request: %s\n",
           uvc_vs_interface_control_name(uvc_request->control),
           uvc_request_code_name(uvc_request->request));

    struct uvc_streaming_control *ctrl = (struct uvc_streaming_control *)&uvc_request->response.data;
    uvc_request->response.length = sizeof(struct uvc_streaming_control);

    switch (uvc_request->request)
    {
    case UVC_SET_CUR:
        uvc_request->set_control = uvc_request->control;
        break;

    case UVC_GET_MAX:
        uvc_fill_streaming_control(processing, ctrl, STREAM_CONTROL_MAX, NULL);
        break;

    case UVC_GET_CUR:
        memcpy(ctrl,
               (uvc_request->control == UVC_VS_PROBE_CONTROL) ? &(uvc->probe) : &(uvc->commit),
               sizeof(struct uvc_streaming_control));
        break;

    case UVC_GET_MIN:
    case UVC_GET_DEF:
        uvc_fill_streaming_control(processing, ctrl, STREAM_CONTROL_MIN, NULL);
        break;

    case UVC_GET_RES:
        memset(&ctrl, 0, sizeof(struct uvc_streaming_control));
        break;

    case UVC_GET_LEN:
        uvc_request->response.length = 2;
        uvc_request->response.data[0] = 0x00;
        uvc_request->response.data[1] = sizeof(struct uvc_streaming_control);
        break;

    case UVC_GET_INFO:
        uvc_request->response.length = 1;
        uvc_request->response.data[0] = (uint8_t)(UVC_CONTROL_CAP_GET | UVC_CONTROL_CAP_SET);
        break;
    }
}

static void uvc_dump_response(struct processing *processing)
{
    struct uvc_request *uvc_request = &processing->uvc_request;
    unsigned int value = 0;
    int i;

    if (uvc_request->response.length == 0)
    {
        printf("UVC: Response EMPTY\n");
    }
    else if (uvc_request->response.length < 0)
    {
        printf("UVC: Response STATUS: %d%s\n",
               uvc_request->response.length,
               (uvc_request->response.length == -EL2HLT) ? " (-EL2HLT)" : ""
        );
    }
    else if (uvc_request->response.length > 0 && uvc_request->response.length < 5)
    {
        memcpy(&value, &uvc_request->response.data[0], uvc_request->response.length);
        printf("UVC: Response value: %d, length: %d\n", value, uvc_request->response.length);
    }
    else
    {
        printf("UVC: Response data, length: %d\n", uvc_request->response.length);
        printf("     ");

        for (i = 0; i < uvc_request->response.length; i++)
        {
            printf("%02x ", uvc_request->response.data[i]);
        }
        printf("\n");
    }
}

static void uvc_events_process_setup(struct processing *processing)
{
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct settings *settings = &processing->settings;
    struct uvc_request *uvc_request = &processing->uvc_request;
    struct uvc_event *uvc_event = (void *)&uvc_request->event.u.data;
    struct usb_ctrlrequest *ctrl = &uvc_event->request;

    uvc_request->set_control = 0;

    if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS &&
        (ctrl->bRequestType & USB_RECIP_MASK) == USB_RECIP_INTERFACE)
    {
        uvc_request->request = ctrl->bRequest;
        uvc_request->type = ctrl->wIndex & 0xff;
        uvc_request->interface = ctrl->wIndex >> 8;
        uvc_request->control = ctrl->wValue >> 8;
        uvc_request->length = ctrl->wLength;

        switch (uvc_request->type)
        {
        case UVC_INTF_CONTROL:
            uvc_interface_control(processing);
            break;

        case UVC_INTF_STREAMING:
            uvc_events_process_streaming(processing);
            break;

        default:
            break;
        }
    }

    if (settings->debug)
    {
        uvc_dump_response(processing);
    }

    if (ioctl(uvc->fd, UVCIOC_SEND_RESPONSE, &uvc_request->response) < 0)
    {
        printf("UVCIOC_SEND_RESPONSE failed: %s (%d)\n", strerror(errno), errno);
    }
}

static void uvc_events_process_data(struct processing *processing)
{
    struct uvc_request *uvc_request = &processing->uvc_request;
    struct uvc_event *uvc_event = (void *)&uvc_request->event.u.data;
    struct uvc_request_data *uvc_request_data = &uvc_event->data;
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct events *events = &processing->events;

    printf("UVC: Control %s, length: %d\n", uvc_vs_interface_control_name(uvc_request->set_control),
           uvc_request_data->length);

    switch (uvc_request->set_control)
    {
    case UVC_VS_PROBE_CONTROL:
        uvc_fill_streaming_control(processing, &(uvc->probe), STREAM_CONTROL_SET,
                                   (struct uvc_streaming_control *)&uvc_request_data->data);
        break;

    case UVC_VS_COMMIT_CONTROL:
        uvc_fill_streaming_control(processing, &(uvc->commit), STREAM_CONTROL_SET,
                                   (struct uvc_streaming_control *)&uvc_request_data->data);
        break;

    case UVC_VS_CONTROL_UNDEFINED:
        if (events->control)
        {
            if (uvc_request_data->length > 0 && uvc_request_data->length <= 4)
            {
                events->control->value = 0x00000000;
                events->control->length = uvc_request_data->length;
                memcpy(&events->control->value, uvc_request_data->data, uvc_request_data->length);
                events->apply_control = true;
            }
            else
            {
                events->control = NULL;
            }
        }
        break;

    default:
        printf("UVC: Control UNKNOWN, length: %d\n", uvc_request_data->length);
        break;
    }
}

void uvc_events_process(struct processing *processing)
{
    struct endpoint_uvc *uvc = &processing->target.uvc;
    struct uvc_request *uvc_request = &processing->uvc_request;

    if (ioctl(uvc->fd, VIDIOC_DQEVENT, &uvc_request->event) < 0)
    {
        printf("UVC: VIDIOC_DQEVENT failed: %s (%d)\n", strerror(errno), errno);
        return;
    }

    memset(&uvc_request->response, 0, sizeof(struct uvc_request_data));
    uvc_request->response.length = -EL2HLT;

    switch (uvc_request->event.type)
    {
    case UVC_EVENT_CONNECT:
        printf("UVC: Event CONNECT\n");
        break;

    case UVC_EVENT_DISCONNECT:
        printf("UVC: Event DISCONNECT\n");
        // processing->events.shutdown_requested = true;
        break;

    case UVC_EVENT_SETUP:
        uvc_events_process_setup(processing);
        break;

    case UVC_EVENT_DATA:
        uvc_events_process_data(processing);
        break;

    case UVC_EVENT_STREAMON:
        printf("UVC: Event STREAM ON\n");
        processing->events.stream = STREAM_ON;
        break;

    case UVC_EVENT_STREAMOFF:
        printf("UVC: Event STREAM OFF\n");
        processing->events.stream = STREAM_OFF;
        break;

    default:
        printf("UVC: Event UNKNOWN\n");
        break;
    }
}

static void uvc_events(struct endpoint_uvc *uvc,
                       int action)
{
    struct v4l2_event_subscription sub;
    memset(&sub, 0, sizeof(struct v4l2_event_subscription));

    sub.type = UVC_EVENT_CONNECT;
    ioctl(uvc->fd, action, &sub);

    sub.type = UVC_EVENT_DISCONNECT;
    ioctl(uvc->fd, action, &sub);

    sub.type = UVC_EVENT_SETUP;
    ioctl(uvc->fd, action, &sub);

    sub.type = UVC_EVENT_DATA;
    ioctl(uvc->fd, action, &sub);

    sub.type = UVC_EVENT_STREAMON;
    ioctl(uvc->fd, action, &sub);

    sub.type = UVC_EVENT_STREAMOFF;
    ioctl(uvc->fd, action, &sub);
}

void uvc_events_subscribe(struct processing *processing)
{
    struct endpoint_uvc *uvc = &processing->target.uvc;

    printf("UVC: Subscribe events\n");
    uvc_events(uvc, VIDIOC_SUBSCRIBE_EVENT);
}

void uvc_events_unsubscribe(struct processing *processing)
{
    struct endpoint_uvc *uvc = &processing->target.uvc;

    printf("UVC: Unsubscribe events\n");
    uvc_events(uvc, VIDIOC_UNSUBSCRIBE_EVENT);
}
