#include <string.h>
#include "queue.h"

void buffer_init(CircularBuffer *buffer)
{
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
}

int buffer_is_empty(CircularBuffer *buffer)
{
    return buffer->count == 0;
}

int buffer_is_full(CircularBuffer *buffer)
{
    return buffer->count == BUFFER_SIZE;
}

int buffer_push(CircularBuffer *buffer, const char *message, size_t length)
{
    if (buffer_is_full(buffer)) {
        return -1;
    }

    if (length >= MESSAGE_SIZE) {
        return -1;
    }

    memcpy(buffer->messages[buffer->tail], message, length);
    buffer->messages[buffer->tail][length] = '\0';
    buffer->lengths[buffer->tail] = length;

    buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
    buffer->count++;

    return 0;
}

int buffer_pop(CircularBuffer *buffer, char *output, size_t *output_length)
{
    if (buffer_is_empty(buffer)) {
        return -1;
    }

    size_t length = buffer->lengths[buffer->head];

    memcpy(output, buffer->messages[buffer->head], length);
    output[length] = '\0';

    if (output_length != NULL) {
        *output_length = length;
    }

    buffer->head = (buffer->head + 1) % BUFFER_SIZE;
    buffer->count--;

    return 0;
}