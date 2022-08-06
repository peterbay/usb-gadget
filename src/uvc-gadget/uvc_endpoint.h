
#ifndef UVC_ENDPOINT
#define UVC_ENDPOINT

#include "headers.h"

void uvc_close(struct processing *processing);

void uvc_init(struct processing *processing,
              const char *device_name,
              unsigned int nbufs);

void uvc_stream_on(struct processing *processing);

void uvc_stream_off(struct processing *processing);

void uvc_apply_format(struct processing *processing);

#endif // end UVC_ENDPOINT
