
#include "headers.h"
#include "control_mapping.h"
#include "uvc_names.h"
#include "v4l2_names.h"

static void v4l2_set_ctrl_value(struct processing *processing,
                                struct control_mapping_pair *ctrl,
                                unsigned int ctrl_v4l2,
                                int v4l2_ctrl_value)
{
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;
    memset(&queryctrl, 0, sizeof(struct v4l2_queryctrl));
    char *control_name = v4l2_control_name(ctrl_v4l2);

    queryctrl.id = ctrl_v4l2;
    if (ioctl(v4l2->fd, VIDIOC_QUERYCTRL, &queryctrl) == -1)
    {
        if (errno != EINVAL)
        {
            printf("V4L2: %s VIDIOC_QUERYCTRL failed: %s (%d).\n",
                   control_name, strerror(errno), errno);
        }
        else
        {
            printf("V4L2: %s is not supported: %s (%d).\n",
                   control_name, strerror(errno), errno);
        }
    }
    else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
    {
        printf("V4L2: %s is disabled.\n", control_name);
    }
    else
    {
        memset(&control, 0, sizeof(struct v4l2_control));
        control.id = ctrl_v4l2;
        control.value = v4l2_ctrl_value;

        if (ioctl(v4l2->fd, VIDIOC_S_CTRL, &control) == -1)
        {
            printf("V4L2: %s VIDIOC_S_CTRL failed: %s (%d).\n",
                   control_name, strerror(errno), errno);
            return;
        }
        if (ctrl != NULL)
        {
            printf("V4L2: %s changed value (V4L2: %d, UVC: %d)\n",
                   control_name, v4l2_ctrl_value, ctrl->value);
        }
        else
        {
            printf("V4L2: %s changed value (V4L2: %d)\n",
                   control_name, v4l2_ctrl_value);
        }
    }
}

void v4l2_set_ctrl(struct processing *processing)
{
    struct events *events = &processing->events;
    struct control_mapping_pair *ctrl = events->control;
    int v4l2_ctrl_value = 0;
    int v4l2_diff = 0;
    int ctrl_diff = 0;

    if (ctrl && events->apply_control)
    {
        v4l2_diff = ctrl->v4l2_maximum - ctrl->v4l2_minimum;
        ctrl_diff = ctrl->maximum - ctrl->minimum;
        ctrl->value = clamp(ctrl->value, ctrl->minimum, ctrl->maximum);
        v4l2_ctrl_value = (ctrl->value - ctrl->minimum) * v4l2_diff / ctrl_diff + ctrl->v4l2_minimum;

        v4l2_set_ctrl_value(processing, ctrl, ctrl->v4l2, v4l2_ctrl_value);

        if (ctrl->v4l2 == V4L2_CID_RED_BALANCE)
        {
            v4l2_set_ctrl_value(processing, ctrl, V4L2_CID_BLUE_BALANCE, v4l2_ctrl_value);
        }

        events->control = NULL;
        events->apply_control = false;
    }
}

static void v4l2_prepare_camera_control(struct processing *processing,
                                        struct control_mapping_pair *mapping,
                                        struct v4l2_queryctrl queryctrl,
                                        unsigned int value)
{
    struct controls *controls = &processing->controls;

    controls->mapping[controls->size] = (struct control_mapping_pair){
        .control_type = queryctrl.type,
        .default_value = (0 - queryctrl.minimum) + queryctrl.default_value,
        .enabled = !(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED),
        .maximum = (0 - queryctrl.minimum) + queryctrl.maximum,
        .minimum = 0,
        .step = queryctrl.step,
        .type = mapping->type,
        .uvc = mapping->uvc,
        .v4l2 = mapping->v4l2,
        .v4l2_maximum = queryctrl.maximum,
        .v4l2_minimum = queryctrl.minimum,
        .value = (0 - queryctrl.minimum) + value,
    };

    printf("V4L2: Mapping %s (%s = %s)\n", queryctrl.name, v4l2_control_name(mapping->v4l2),
           uvc_control_name(mapping->type, mapping->uvc));

    printf("V4L2:   V4L2: min: %d, max: %d, step: %d, default: %d, value: %d\n",
           queryctrl.minimum,
           queryctrl.maximum,
           queryctrl.step,
           queryctrl.default_value,
           value);

    printf("V4L2:   UVC:  min: %d, max: %d, step: %d, default: %d, value: %d\n",
           controls->mapping[controls->size].minimum,
           controls->mapping[controls->size].maximum,
           controls->mapping[controls->size].step,
           controls->mapping[controls->size].default_value,
           controls->mapping[controls->size].value);

    controls->size += 1;
}

void v4l2_apply_stored_controls(struct processing *processing)
{
    struct stored_controls *stored_controls = processing->stored_controls;
    struct stored_control *stored_control;
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    int i;

    for (i = 0; i < stored_controls->length; i++)
    {
        stored_control = &stored_controls->controls[i];
        if (!stored_control->v4l2) {
            continue;
        }
        if ((stored_control->apply_type == CONTROL_APPLY_ONCE && v4l2->stream_on_count == 1) ||
            stored_control->apply_type == CONTROL_APPLY_ALWAYS)
        {
            printf("V4L2: Apply stored control: name: %s, apply: %s\n",
                   stored_control->control_name,
                   (stored_control->apply_type == CONTROL_APPLY_ONCE) ? "ONCE" : "ALWAYS");
            v4l2_set_ctrl_value(processing, NULL, stored_control->v4l2,  stored_control->value);
        }
    }
}

void v4l2_parse_stored_controls(struct processing *processing)
{
    struct stored_controls *stored_controls = processing->stored_controls;
    struct stored_control *stored_control;
    int i;
    int value;
    char *ptr_name;
    char *ptr_value;

    for (i = 0; i < stored_controls->length; i++)
    {
        stored_control = &stored_controls->controls[i];
        printf("V4L2: Parsing of control: %s\n", stored_control->args);
        ptr_name = strtok(stored_control->args, "=");

        if (ptr_name != NULL)
        {
            ptr_value = strtok(NULL, "=");
            if (ptr_value != NULL)
            {
                sscanf(ptr_value, "%d", &value);
                stored_control->control_name = ptr_name;
                stored_control->control_value = ptr_value;
                stored_control->value = value;

                printf("V4L2: Control parsed: name: %s, value: %d, apply: %s\n",
                       stored_control->control_name,
                       stored_control->value,
                       (stored_control->apply_type == CONTROL_APPLY_ONCE) ? "ONCE" : "ALWAYS");
            }
            else
            {
                printf("V4L2: ERROR: Control parsed: value not found\n");
            }
        }
        else
        {
            printf("V4L2: ERROR: Control parsed: delimiter '=' not found\n");
        }
    }
}

int v4l2_check_control_name(char *name, char *check_name)
{
    int i;
    int len_name = strlen(name) - 1;
    char lowercase;
    char out_name[127] = {'\0'};
    bool add_underscore = false;

    for (i = 0; i <= len_name; i++)
    {
        if (isalnum(name[i]))
        {
            if (add_underscore)
            {
                strcat(out_name, "_");
                add_underscore = false;
            }
            lowercase = tolower(name[i]);
            strncat(out_name, &lowercase, 1);
        }
        else
        {
            add_underscore = true;
        }
    }
    return strcmp(out_name, check_name);
}

void v4l2_init_stored_controls(struct processing *processing)
{
    struct stored_controls *stored_controls = processing->stored_controls;
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    struct v4l2_queryctrl queryctrl;
    int i;

    const unsigned next_fl = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;

    if (stored_controls->length == 0) {
        return;
    }

    memset(&queryctrl, 0, sizeof(struct v4l2_queryctrl));

    queryctrl.id = next_fl;
    while (ioctl(v4l2->fd, VIDIOC_QUERYCTRL, &queryctrl) == 0)
    {
        for (i = 0; i < stored_controls->length; i++)
        {
            if (v4l2_check_control_name((char *)queryctrl.name, (char *)stored_controls->controls[i].control_name) == 0)
            {
                printf ("V4L2: Found V4L2 control for stored control %s = %s (%s)\n",
                        stored_controls->controls[i].control_name,
                        queryctrl.name,
                        v4l2_control_name(queryctrl.id));
                stored_controls->controls[i].v4l2 = queryctrl.id;
                break;
            }
        }
        queryctrl.id |= next_fl;
    }
}

void v4l2_get_controls(struct processing *processing)
{
    struct controls *controls = &processing->controls;
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control ctrl;
    int i;

    const unsigned next_fl = V4L2_CTRL_FLAG_NEXT_CTRL | V4L2_CTRL_FLAG_NEXT_COMPOUND;

    memset(&queryctrl, 0, sizeof(struct v4l2_queryctrl));

    controls->size = 0;

    queryctrl.id = next_fl;
    while (ioctl(v4l2->fd, VIDIOC_QUERYCTRL, &queryctrl) == 0)
    {
        for (i = 0; i < control_mapping_size; i++)
        {
            if (control_mapping[i].v4l2 == queryctrl.id)
            {
                ctrl.id = queryctrl.id;
                if (ioctl(v4l2->fd, VIDIOC_G_CTRL, &ctrl) == 0)
                {
                    v4l2_prepare_camera_control(processing, &control_mapping[i], queryctrl,
                                                ctrl.value);
                }
            }
        }
        queryctrl.id |= next_fl;
    }
}

static void v4l2_get_available_formats(struct endpoint_v4l2 *v4l2)
{
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_frmsizeenum frmsize;

    memset(&fmtdesc, 0, sizeof(struct v4l2_fmtdesc));
    memset(&frmsize, 0, sizeof(struct v4l2_frmsizeenum));

    v4l2->jpeg_format_available = false;

    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while (ioctl(v4l2->fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0)
    {
        fmtdesc.index++;
        frmsize.pixel_format = fmtdesc.pixelformat;
        frmsize.index = 0;
        while (ioctl(v4l2->fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0)
        {
            if (fmtdesc.pixelformat == V4L2_PIX_FMT_JPEG)
            {
                v4l2->jpeg_format_available = true;
            }

            if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
            {
                printf("V4L2: Highest frame size: %c%c%c%c %ux%u\n",
                       pixfmtstr(fmtdesc.pixelformat), frmsize.discrete.width, frmsize.discrete.height);
            }
            else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE)
            {
                printf("V4L2: Highest frame size: %c%c%c%c %ux%u\n",
                       pixfmtstr(fmtdesc.pixelformat), frmsize.stepwise.max_width, frmsize.stepwise.max_height);
            }

            frmsize.index++;
        }
    }
}

void v4l2_stream_on(struct processing *processing)
{
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    struct v4l2_requestbuffers req;
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    unsigned int i = 0;

    if (processing->source.type == ENDPOINT_V4L2 &&
        v4l2->is_streaming == false)
    {
        printf("V4L2: Stream ON init\n");

        v4l2->dqbuf_count = 0;
        v4l2->qbuf_count = 0;

        memset(&req, 0, sizeof(struct v4l2_requestbuffers));
        req.count = v4l2->nbufs;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (ioctl(v4l2->fd, VIDIOC_REQBUFS, &req) < 0)
        {
            printf("V4L2: VIDIOC_REQBUFS error: %s (%d).\n", strerror(errno), errno);
            return;
        }

        v4l2->mem = calloc(req.count, sizeof v4l2->mem[0]);
        if (!v4l2->mem)
        {
            printf("V4L2: Out of memory\n");
            return;
        }

        for (i = 0; i < req.count; ++i)
        {
            memset(&v4l2->mem[i].buf, 0, sizeof(struct v4l2_buffer));
            v4l2->mem[i].buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            v4l2->mem[i].buf.memory = V4L2_MEMORY_MMAP;
            v4l2->mem[i].buf.index = i;

            if (ioctl(v4l2->fd, VIDIOC_QUERYBUF, &(v4l2->mem[i].buf)) < 0)
            {
                printf("V4l2: VIDIOC_QUERYBUF failed for buf %d: %s (%d).\n", i, strerror(errno), errno);
                free(v4l2->mem);
                return;
            }

            v4l2->mem[i].start = mmap(NULL, v4l2->mem[i].buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                                      v4l2->fd, v4l2->mem[i].buf.m.offset);

            if (MAP_FAILED == v4l2->mem[i].start)
            {
                printf("V4L2: Unable to map buffer %u: %s (%d).\n", i, strerror(errno), errno);
                v4l2->mem[i].length = 0;
                free(v4l2->mem);
                return;
            }

            v4l2->mem[i].length = v4l2->mem[i].buf.length;
            printf("V4L2: Buffer %u mapped at address %p, length %d.\n", i, v4l2->mem[i].start,
                   v4l2->mem[i].length);
        }
        printf("V4L2: %u buffers allocated.\n", req.count);

        for (i = 0; i < v4l2->nbufs; ++i)
        {
            memset(&v4l2->mem[i].buf, 0, sizeof(struct v4l2_buffer));
            v4l2->mem[i].buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            v4l2->mem[i].buf.memory = V4L2_MEMORY_MMAP;
            v4l2->mem[i].buf.index = i;

            if (ioctl(v4l2->fd, VIDIOC_QBUF, &(v4l2->mem[i].buf)) < 0)
            {
                printf("V4L2: VIDIOC_QBUF failed : %s (%d).\n", strerror(errno), errno);
                return;
            }
            v4l2->qbuf_count++;
        }

        if (ioctl(v4l2->fd, VIDIOC_STREAMON, &type) < 0)
        {
            printf("V4L2: STREAM ON failed: %s (%d).\n", strerror(errno), errno);
            return;
        }

        printf("V4L2: STREAM ON success\n");
        v4l2->stream_on_count += 1;
        v4l2->is_streaming = 1;

        v4l2_apply_stored_controls(processing);
    }
}

void v4l2_stream_off(struct processing *processing)
{
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_requestbuffers req;
    unsigned int i;

    if (processing->source.type == ENDPOINT_V4L2 &&
        v4l2->is_streaming == true)
    {
        printf("V4L2: Stream OFF init\n");

        if (ioctl(v4l2->fd, VIDIOC_STREAMOFF, &type) < 0)
        {
            printf("V4L2: STREAM OFF failed: %s (%d).\n", strerror(errno), errno);
            return;
        }

        if (v4l2->mem)
        {
            printf("V4L2: Uninit device\n");

            for (i = 0; i < v4l2->nbufs; ++i)
            {
                if (munmap(v4l2->mem[i].start, v4l2->mem[i].length) < 0)
                {
                    printf("V4L2: munmap failed\n");
                    return;
                }
            }
            free(v4l2->mem);
            v4l2->mem = NULL;
        }

        memset(&req, 0, sizeof(struct v4l2_requestbuffers));
        req.count = 0;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (ioctl(v4l2->fd, VIDIOC_REQBUFS, &req) < 0)
        {
            printf("V4L2: VIDIOC_REQBUFS error: %s (%d).\n", strerror(errno), errno);
            return;
        }

        printf("V4L2: Stream OFF success\n");
        v4l2->is_streaming = false;
    }
}

void v4l2_fps_set(struct processing *processing)
{
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    struct events *events = &processing->events;
    static struct v4l2_streamparm parm;
    int fps;
    memset(&parm, 0, sizeof(parm));

    if (processing->source.type == ENDPOINT_V4L2 && events->apply_frame_format)
    {
        if (events->apply_frame_format->dwDefaultFrameInterval > 0)
        {
            fps = (int) 10000000 / events->apply_frame_format->dwDefaultFrameInterval;

            parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            parm.parm.capture.timeperframe.numerator = 1000;
            parm.parm.capture.timeperframe.denominator =
                (uint32_t)(fps * parm.parm.capture.timeperframe.numerator);

            printf("V4L2: Setting FPS: %d\n", fps);

            if (ioctl(v4l2->fd, VIDIOC_S_PARM, &parm) < 0)
            {
                printf("V4L2: VIDIOC_S_PARM error: %s (%d)\n", strerror(errno), errno);
            }
        }
    }
}

void v4l2_apply_format(struct processing *processing)
{
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    struct events *events = &processing->events;
    struct v4l2_format fmt;
    int video_format;

    if (processing->source.type == ENDPOINT_V4L2 && events->apply_frame_format)
    {
        if (v4l2->jpeg_format_available &&
            v4l2->jpeg_format_use &&
            events->apply_frame_format->video_format == V4L2_PIX_FMT_MJPEG)
        {
            video_format = V4L2_PIX_FMT_JPEG;
        }
        else
        {
            video_format = events->apply_frame_format->video_format;
        }

        memset(&fmt, 0, sizeof(struct v4l2_format));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.field = V4L2_FIELD_ANY;
        fmt.fmt.pix.width = events->apply_frame_format->wWidth;
        fmt.fmt.pix.height = events->apply_frame_format->wHeight;
        fmt.fmt.pix.pixelformat = video_format;
        fmt.fmt.pix.sizeimage = fmt.fmt.pix.width * fmt.fmt.pix.height *
                                ((fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV) ? 2 : 1);

        if (ioctl(v4l2->fd, VIDIOC_S_FMT, &fmt) < 0)
        {
            printf("V4L2: Unable to set format %s (%d).\n", strerror(errno), errno);
            return;
        }

        printf("V4L2: Setting format: %c%c%c%c %ux%u\n", pixfmtstr(fmt.fmt.pix.pixelformat),
               fmt.fmt.pix.width, fmt.fmt.pix.height);

        memset(&fmt, 0, sizeof(struct v4l2_format));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (ioctl(v4l2->fd, VIDIOC_G_FMT, &fmt) < 0)
        {
            printf("V4L2: Unable to get format %s (%d).\n", strerror(errno), errno);
            return;
        }

        printf("V4L2: Getting format: %c%c%c%c %ux%u\n", pixfmtstr(fmt.fmt.pix.pixelformat),
               fmt.fmt.pix.width, fmt.fmt.pix.height);
    }
}

void v4l2_close(struct processing *processing)
{
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;

    if (processing->source.type == ENDPOINT_V4L2 &&
        v4l2->fd)
    {
        printf("V4L2: Closing %s device\n", v4l2->device_name);
        close(v4l2->fd);
        v4l2->fd = -1;
    }
}

void v4l2_init(struct processing *processing,
               const char *device_name,
               unsigned int nbufs,
               bool jpeg_format_use)
{
    struct endpoint_v4l2 *v4l2 = &processing->source.v4l2;
    struct settings *settings = &processing->settings;
    struct v4l2_capability cap;

    if (processing->source.type == ENDPOINT_NONE && device_name)
    {
        printf("V4L2: Opening %s device\n", device_name);

        processing->source.state = true;
        v4l2->device_name = device_name;
        v4l2->fd = open(device_name, O_RDWR | O_NONBLOCK, 0);
        if (v4l2->fd == -1)
        {
            printf("V4L2: Device open failed: %s (%d).\n", strerror(errno), errno);
            return;
        }

        if (ioctl(v4l2->fd, VIDIOC_QUERYCAP, &cap) < 0)
        {
            printf("V4L2: VIDIOC_QUERYCAP failed: %s (%d).\n", strerror(errno), errno);
            goto err;
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
        {
            printf("V4L2: %s is no video capture device\n", device_name);
            goto err;
        }

        if (!(cap.capabilities & V4L2_CAP_STREAMING))
        {
            printf("V4L2: %s does not support streaming i/o\n", device_name);
            goto err;
        }

        printf("V4L2: Device is %s on bus %s\n", cap.card, cap.bus_info);

        print_v4l2_capabilities("V4L2", cap.capabilities);

        v4l2->nbufs = nbufs;
        v4l2->jpeg_format_use = jpeg_format_use;
        v4l2->stream_on_count = 0;
        processing->source.type = ENDPOINT_V4L2;
        processing->source.state = true;

        if (!settings->ignore_camera_controls)
        {
            v4l2_get_controls(processing);
        }

        v4l2_parse_stored_controls(processing);
        v4l2_init_stored_controls(processing);

        v4l2_get_available_formats(v4l2);

        if (v4l2->jpeg_format_use)
        {
            if (v4l2->jpeg_format_available)
            {
                printf("V4L2: JPEG format required and available\n");
            }
            else
            {
                printf("V4L2: WARNING: JPEG format required but NOT available\n");
            }
        }

        return;
    }

err:
    v4l2_close(processing);
};
