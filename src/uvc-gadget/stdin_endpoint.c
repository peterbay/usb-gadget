
#include "headers.h"
#include "stdin_endpoint.h"
#include "data_buffers.h"

void fill_buffer_from_stdin(struct processing *processing)
{
    struct endpoint_stdin *stdin = &processing->source.stdin;
    struct data_buffers *data_buffers = &processing->data_buffers;
    struct data_buffer *data_buffer = data_buffers->buffer_fill;
    struct settings *settings = &processing->settings;

    unsigned char c = 0;
    unsigned char c_prev = 0;
    unsigned int bytesused = data_buffer->bytesused;
    unsigned int limit = data_buffer->length - 1;
    unsigned int frame_size = stdin->width * stdin->height * 2;
    unsigned int readed = 0;

    if (stdin->stdin_format == V4L2_PIX_FMT_MJPEG)
    {
        if (bytesused > 0)
        {
            memcpy(&c, data_buffer + bytesused - 1, 1);
        }

        while (read(STDIN_FILENO, &c, sizeof(c)) > 0)
        {
            memcpy(data_buffer->start + bytesused, &c, 1);
            bytesused += 1;

            if (bytesused == 2)
            {
                if (c != 0xD8 && c_prev != 0xFF)
                {
                    bytesused = 0;
                    data_buffer->filled = false;
                    data_buffer->bytesused = 0;
                }
            }
            else if (c_prev == 0xFF)
            {
                if (c == 0xD9)
                {
                    buffers_swap(processing);
                    break;
                }
            }

            if (bytesused >= limit)
            {
                bytesused = 0;
            }

            c_prev = c;
        };
    }
    else
    {
        do {
            readed = read(STDIN_FILENO, data_buffer->start + bytesused, frame_size - bytesused);
            bytesused += readed;

            if (bytesused == frame_size)
            {
                buffers_swap(processing);
                break;
            }
        }
        while (readed > 0);
    }

    data_buffer->bytesused = bytesused;
    if (settings->debug)
    {
        printf("STDIN: Frame readed %d bytes\n", bytesused);
    }
}

void stdin_get_first_frame(struct processing *processing)
{
    struct endpoint_stdin *stdin = &processing->source.stdin;
    struct data_buffers *data_buffers = &processing->data_buffers;
    struct data_buffer *data_buffer = data_buffers->buffer_fill;
    struct events *events = &processing->events;

    int activity;
    struct timeval tv;
    fd_set fdsi;

    printf("STDIN: Waiting for first frame\n");

    while (!*(events->terminate) && !data_buffer->filled)
    {
        FD_ZERO(&fdsi);
        FD_SET(STDIN_FILENO, &fdsi);
 
        nanosleep((const struct timespec[]){{0, 1000000L}}, NULL);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        activity = select(STDIN_FILENO + 1, &fdsi, NULL, NULL, &tv);

        if (activity == -1)
        {
            printf("STDIN: Select error %d, %s\n", errno, strerror(errno));
            if (errno == EINTR)
            {
                continue;
            }
            break;
        }

        if (activity == 0)
        {
            printf("STDIN: Select timeout\n");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &fdsi))
        {
            fill_buffer_from_stdin(processing);
        }
    }
}

void stdin_init(struct processing *processing,
                unsigned int stdin_format,
                unsigned int width,
                unsigned int height
                )
{
    struct endpoint_stdin *stdin = &processing->source.stdin;
    struct data_buffers *data_buffers = &processing->data_buffers;
    struct settings *settings = &processing->settings;
    unsigned int buffer_size;
    int ret;

    if (processing->source.type == ENDPOINT_NONE && stdin_format)
    {
        printf("STDIN: Initialize stdin\n");

        stdin->stdin_format = stdin_format;
        stdin->width = width;
        stdin->height = height;

        buffer_size = width * height * 2;

        ret = buffers_init(processing, buffer_size);
        if (ret == -1)
        {
            return;
        }

        stdin_get_first_frame(processing);

        if (!data_buffers->buffer_fill)
        {
            buffers_free(processing);
            return;
        }

        data_buffers->fill_buffer = true;
        processing->source.type = ENDPOINT_STDIN;
        processing->source.state = true;
        settings->uvc_buffer_required = true;
        settings->uvc_buffer_size = buffer_size;
    }
}
