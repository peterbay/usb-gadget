
#ifndef STDIN_ENDPOINT
#define STDIN_ENDPOINT

#include "headers.h"

int stdin_buffer_init(struct processing *processing);

void stdin_buffer_free(struct processing *processing);

void fill_buffer_from_stdin(struct processing *processing);

void stdin_get_first_frame(struct processing *processing);

void stdin_init(struct processing *processing,
                unsigned int stdin_format,
                unsigned int width,
                unsigned int height
    );

#endif // end STDIN_ENDPOINT
