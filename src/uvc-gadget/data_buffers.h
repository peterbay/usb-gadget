#ifndef DATA_BUFFERS
#define DATA_BUFFERS

int buffers_init(struct processing *processing,
                 unsigned int buffer_size);

void buffers_free(struct processing *processing);

void buffers_swap(struct processing *processing);

#endif // end DATA_BUFFERS
