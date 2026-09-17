#ifndef SHARED_H
#define SHARED_H

#include <pthread.h>
#include <signal.h>
#include "queue.h"

typedef struct {
    CircularBuffer buffer;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;

    int producer_done;
    int consumer_done;

    int commit_count;
    int identity_count;
    int account_count;
    int info_count;

    unsigned long dropped_messages;
    unsigned long parse_errors;
} SharedData;

extern volatile sig_atomic_t stop_requested;

#endif