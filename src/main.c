#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <libwebsockets.h>

#include "shared.h"
#include "queue.h"
#include "producer.h"
#include "consumer.h"
#include "monitor.h"

volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int sig)
{
    (void)sig;
    stop_requested = 1;
}

int main(void)
{
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    lws_set_log_level(LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);

    SharedData shared;
    memset(&shared, 0, sizeof(shared));

    pthread_t producer;
    pthread_t consumer;
    pthread_t monitor;

    buffer_init(&shared.buffer);

    pthread_mutex_init(&shared.mutex, NULL);
    pthread_cond_init(&shared.not_empty, NULL);

    printf("Main: ξεκινάω Producer, Consumer και Monitor threads.\n");
    printf("Πάτα Ctrl+C για να σταματήσει καθαρά.\n");

    pthread_create(&producer, NULL, producer_thread, &shared);
    pthread_create(&consumer, NULL, consumer_thread, &shared);
    pthread_create(&monitor, NULL, monitor_thread, &shared);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    pthread_join(monitor, NULL);

    pthread_mutex_lock(&shared.mutex);

    unsigned long dropped = shared.dropped_messages;
    unsigned long parse_errors = shared.parse_errors;

    pthread_mutex_unlock(&shared.mutex);

    pthread_mutex_destroy(&shared.mutex);
    pthread_cond_destroy(&shared.not_empty);

    printf("\nMain: πρόγραμμα τελείωσε.\n");
    printf("Dropped messages: %lu\n", dropped);
    printf("Parse errors: %lu\n", parse_errors);
    printf("Το CSV βρίσκεται στο metrics_log.txt\n");

    return 0;
}