#include "headers.h"

int buffers_init(struct processing *processing,
                 unsigned int buffer_size)
{
    struct data_buffers *data_buffers = &processing->data_buffers;
    unsigned int i;

    printf("BUFFERS: Initialize buffers\n");

    data_buffers->buffers = calloc(2, sizeof data_buffers->buffers[0]);
    if (!data_buffers->buffers)
    {
        printf("BUFFERS: Out of memory\n");
        return -1;
    }

    data_buffers->buffer_size = buffer_size;

    for (i = 0; i < 2; ++i)
    {
        data_buffers->buffers[i].index = i;
        data_buffers->buffers[i].filled = false;
        data_buffers->buffers[i].length = data_buffers->buffer_size;
        data_buffers->buffers[i].start = malloc(data_buffers->buffer_size);
        if (!data_buffers->buffers[i].start)
        {
            printf("BUFFERS: Out of memory\n");
            return -1;
        }
    }

    data_buffers->buffer_fill = &data_buffers->buffers[0];
    data_buffers->buffer_use = &data_buffers->buffers[0];

    printf("BUFFERS: Initialized\n");

    return 0;
}

void buffers_free(struct processing *processing)
{
    struct data_buffers *data_buffers = &processing->data_buffers;
    unsigned int i;

    if (data_buffers->buffers)
    {
        printf("BUFFERS: Free buffers\n");

        for (i = 0; i < 2; ++i)
        {
            free(data_buffers->buffers[i].start);
            data_buffers->buffers[i].start = NULL;
        }
        free(data_buffers->buffers);
        data_buffers->buffers = NULL;
    }
}

void buffers_swap(struct processing *processing)
{
    struct data_buffers *data_buffers = &processing->data_buffers;
    struct settings *settings = &processing->settings;

    unsigned int index_fill = (data_buffers->buffer_fill->index == 0) ? 1 : 0;

    data_buffers->buffer_use = data_buffers->buffer_fill;
    data_buffers->buffer_use->filled = true;

    data_buffers->buffer_fill = &data_buffers->buffers[index_fill];

    data_buffers->buffer_fill->filled = false;
    data_buffers->buffer_fill->bytesused = 0;

    data_buffers->fill_buffer = false;

    if (settings->debug)
    {
        printf("BUFFERS: Buffers swapped, now fill the buffer #%d\n", index_fill);
    }
}