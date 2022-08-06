
#ifndef FB_ENDPOINT
#define FB_ENDPOINT

#include "headers.h"

int fb_mmap_open(struct processing *processing);

void fb_mmap_close(struct processing *processing);

void fb_close(struct processing *processing);

void fb_init(struct processing *processing,
             const char *device_name);

void fb_stream_on(struct processing *processing);

void fb_stream_off(struct processing *processing);

#endif // end FB_ENDPOINT
