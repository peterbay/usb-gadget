
#include "headers.h"
#include "image_endpoint.h"
#include "data_buffers.h"

int image_load(struct processing *processing)
{
    struct endpoint_image *image = &processing->source.image;
    struct data_buffers *data_buffers = &processing->data_buffers;
    struct data_buffer *data_buffer = data_buffers->buffer_fill;
    int fd;
    size_t size;
    size_t readed;

    fd = open(image->image_path, O_RDONLY);
    if (fd == -1)
    {
        printf("IMAGE: Unable to open image '%s'\n", image->image_path);
        return -1;
    }

    size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    if (size <= data_buffer->length)
    {
        readed = read(fd, data_buffer->start, size);
        if (readed == size)
        {
            data_buffer->bytesused = readed;
            buffers_swap(processing);
        }
        else
        {
            printf("IMAGE: ERROR: Unable to read image\n");
            goto err;
        }
    }
    else
    {
        printf("IMAGE: ERROR: Image length is higher than allocated memory\n");
        goto err;
    }

    close(fd);

    return 0;

err:
    if (fd)
    {
        close(fd);
    }
    return -1;
}

void image_close(struct processing *processing)
{
    struct endpoint_image *image = &processing->source.image;

    if (processing->source.type == ENDPOINT_IMAGE)
    {
        printf("IMAGE: Closing image\n");
        inotify_rm_watch(image->inotify_fd, IN_CLOSE_WRITE);
    }
}

void image_init(struct processing *processing,
                const char *image_path)
{
    struct endpoint_image *image = &processing->source.image;
    struct settings *settings = &processing->settings;

    if (processing->source.type == ENDPOINT_NONE && image_path)
    {
        printf("IMAGE: Opening image %s\n", image_path);

        image->image_path = image_path;

        if (buffers_init(processing, 4147200) == -1)
        {
            return;
        }

        if (image_load(processing) == -1)
        {
            return;
        }

        image->inotify_fd = inotify_init();
        inotify_add_watch(image->inotify_fd, image_path, IN_CLOSE_WRITE);

        processing->source.type = ENDPOINT_IMAGE;
        processing->source.state = true;
        settings->uvc_buffer_required = true;
        settings->uvc_buffer_size = 4147200;
    }
}
