#ifndef QUEUE_H
#define QUEUE_H

#include <stddef.h>
#include "config.h"

typedef struct {
    char messages[BUFFER_SIZE][MESSAGE_SIZE];
    size_t lengths[BUFFER_SIZE];
    int head;
    int tail;
    int count;
} CircularBuffer;

void buffer_init(CircularBuffer *buffer);

int buffer_is_empty(CircularBuffer *buffer);

int buffer_is_full(CircularBuffer *buffer);

int buffer_push(CircularBuffer *buffer, const char *message, size_t length);

int buffer_pop(CircularBuffer *buffer, char *output, size_t *output_length);

#endif