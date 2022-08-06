
#ifndef UVC_CONTROL
#define UVC_CONTROL

#include "headers.h"

void uvc_fill_streaming_control(struct processing *processing,
                                struct uvc_streaming_control *ctrl_out,
                                enum stream_control_action action,
                                struct uvc_streaming_control *ctrl_in);

void uvc_dump_streaming_control(struct uvc_streaming_control *usc);

#endif // end UVC_CONTROL
