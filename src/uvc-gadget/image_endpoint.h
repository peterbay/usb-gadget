
#ifndef IMAGE_ENDPOINT
#define IMAGE_ENDPOINT

#include "headers.h"

#define INOTIFY_BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

void image_init(struct processing *processing,
                const char *image_path);

void image_close(struct processing *processing);

int image_load(struct processing *processing);

#endif // end IMAGE_ENDPOINT
