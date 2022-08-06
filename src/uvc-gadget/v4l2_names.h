#ifndef V4L2_NAMES
#define V4L2_NAMES

#include "headers.h"

char *v4l2_control_name(unsigned int control);

void print_v4l2_capabilities(const char *title,
                             uint32_t cap);

#endif // end V4L2_NAMES
