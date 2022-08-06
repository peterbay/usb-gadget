
#ifndef UVC_NAMES
#define UVC_NAMES

#include <linux/usb/video.h>

char *uvc_request_code_name(unsigned int uvc_control);

char *uvc_vs_interface_control_name(unsigned int interface);

char *uvc_processing_control_name(unsigned int pu_control);

char *uvc_terminal_control_name(unsigned int terminal_control);

char *uvc_control_name(unsigned int control_type,
                       unsigned int control);

#endif // end UVC_NAMES
