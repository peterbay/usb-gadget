
#ifndef V4L2_ENDPOINT
#define V4L2_ENDPOINT

#include "headers.h"

void v4l2_close(struct processing *processing);

void v4l2_init(struct processing *processing,
               const char *device_name,
               unsigned int nbufs,
               bool jpeg_format_use);

void v4l2_stream_on(struct processing *processing);

void v4l2_stream_off(struct processing *processing);

void v4l2_apply_format(struct processing *processing);

void v4l2_set_ctrl(struct processing *processing);

void v4l2_fps_set(struct processing *processing);

#endif // end V4L2_ENDPOINT
