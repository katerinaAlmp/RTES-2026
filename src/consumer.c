#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "shared.h"
#include "queue.h"
#include "consumer.h"

static void increment_kind_counter(SharedData *shared, const char *kind)
{
    pthread_mutex_lock(&shared->mutex);

    if (strcmp(kind, "commit") == 0) {
        shared->commit_count++;
    } else if (strcmp(kind, "identity") == 0) {
        shared->identity_count++;
    } else if (strcmp(kind, "account") == 0) {
        shared->account_count++;
    } else if (strcmp(kind, "info") == 0) {
        shared->info_count++;
    }

    pthread_mutex_unlock(&shared->mutex);
}

static void process_json_message(SharedData *shared, const char *message, size_t length)
{
    cJSON *root = cJSON_ParseWithLength(message, length);

    if (root == NULL) {
        pthread_mutex_lock(&shared->mutex);
        shared->parse_errors++;
        pthread_mutex_unlock(&shared->mutex);
        return;
    }

    cJSON *kind = cJSON_GetObjectItemCaseSensitive(root, "kind");

    if (cJSON_IsString(kind) && kind->valuestring != NULL) {
        increment_kind_counter(shared, kind->valuestring);
    }

    cJSON_Delete(root);
}

void *consumer_thread(void *arg)
{
    SharedData *shared = (SharedData *)arg;
    char message[MESSAGE_SIZE];
    size_t message_length;

    while (1) {
        pthread_mutex_lock(&shared->mutex);

        while (buffer_is_empty(&shared->buffer) && !shared->producer_done) {
            pthread_cond_wait(&shared->not_empty, &shared->mutex);
        }

        if (!buffer_is_empty(&shared->buffer)) {
            buffer_pop(&shared->buffer, message, &message_length);

            pthread_mutex_unlock(&shared->mutex);

            process_json_message(shared, message, message_length);
        } else if (shared->producer_done) {
            shared->consumer_done = 1;

            pthread_mutex_unlock(&shared->mutex);
            break;
        }
    }

    printf("Consumer τελείωσε.\n");

    return NULL;
}