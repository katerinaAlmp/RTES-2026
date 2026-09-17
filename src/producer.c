#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libwebsockets.h>

#include "config.h"
#include "shared.h"
#include "queue.h"
#include "producer.h"

/*
 * Το αρχείο αυτό υπάρχει ήδη στο Raspberry Pi.
 * Χρησιμοποιείται από την MbedTLS για την επαλήθευση
 * του πιστοποιητικού του Jetstream server.
 */
#define CA_BUNDLE_PATH "/etc/ssl/certs/ca-certificates.crt"

typedef struct {
    SharedData *shared;
    int connection_closed;
} ProducerState;


static void sleep_milliseconds(long milliseconds)
{
    struct timespec ts;

    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000L;

    nanosleep(&ts, NULL);
}


static int websocket_callback(struct lws *wsi,
                              enum lws_callback_reasons reason,
                              void *user,
                              void *in,
                              size_t len)
{
    (void)user;

    ProducerState *state =
        (ProducerState *)lws_context_user(lws_get_context(wsi));

    if (state == NULL || state->shared == NULL) {
        return 0;
    }

    SharedData *shared = state->shared;

    switch (reason) {

        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("Producer: WebSocket συνδέθηκε επιτυχώς.\n");
            break;


        case LWS_CALLBACK_CLIENT_RECEIVE:
            /*
             * Ο Producer κάνει μόνο την ελάχιστη εργασία:
             * τοποθετεί το JSON στην ουρά και ξυπνά τον Consumer.
             */
            pthread_mutex_lock(&shared->mutex);

            if (buffer_push(&shared->buffer,
                            (const char *)in,
                            len) == 0) {

                pthread_cond_signal(&shared->not_empty);

            } else {
                shared->dropped_messages++;
            }

            pthread_mutex_unlock(&shared->mutex);
            break;


        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            /*
             * Το in περιέχει την πραγματική αιτία του σφάλματος.
             * Χρησιμοποιούμε %.*s επειδή δεν είναι εγγυημένο ότι
             * το μήνυμα τελειώνει με χαρακτήρα '\0'.
             */
            if (in != NULL && len > 0) {
                fprintf(stderr,
                        "Producer: WebSocket connection error: %.*s\n",
                        (int)len,
                        (const char *)in);
            } else {
                fprintf(stderr,
                        "Producer: WebSocket connection error "
                        "χωρίς επιπλέον πληροφορίες.\n");
            }

            state->connection_closed = 1;
            break;


        case LWS_CALLBACK_CLIENT_CLOSED:
            printf("Producer: WebSocket έκλεισε.\n");

            state->connection_closed = 1;
            break;


        default:
            break;
    }

    return 0;
}


static struct lws_protocols protocols[] = {
    {
        "jetstream-client",
        websocket_callback,
        0,
        MESSAGE_SIZE
    },
    {
        NULL,
        NULL,
        0,
        0
    }
};


void *producer_thread(void *arg)
{
    SharedData *shared = (SharedData *)arg;

    while (!stop_requested) {

        ProducerState state;
        memset(&state, 0, sizeof(state));

        state.shared = shared;
        state.connection_closed = 0;


        /*
         * Δημιουργία του libwebsockets context.
         */
        struct lws_context_creation_info info;
        memset(&info, 0, sizeof(info));

        info.port = CONTEXT_PORT_NO_LISTEN;
        info.protocols = protocols;
        info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        info.user = &state;

        /*
         * Απαραίτητο για την ασφαλή επαλήθευση του
         * πιστοποιητικού από την MbedTLS.
         */
        info.client_ssl_ca_filepath = CA_BUNDLE_PATH;


        struct lws_context *context =
            lws_create_context(&info);

        if (context == NULL) {
            fprintf(stderr,
                    "Producer: αποτυχία δημιουργίας lws context. "
                    "Ξαναδοκιμάζω σε 2 sec.\n");

            sleep_milliseconds(2000);
            continue;
        }


        /*
         * Στοιχεία σύνδεσης προς το Jetstream.
         */
        struct lws_client_connect_info ccinfo;
        memset(&ccinfo, 0, sizeof(ccinfo));

        ccinfo.context = context;
        ccinfo.address = WS_ADDRESS;
        ccinfo.port = WS_PORT;
        ccinfo.path = WS_PATH;
        ccinfo.host = WS_ADDRESS;
        ccinfo.origin = WS_ADDRESS;
        ccinfo.ssl_connection = LCCSCF_USE_SSL;

        /*
         * Το Jetstream δεν απαιτεί WebSocket subprotocol.
         */
        ccinfo.protocol = NULL;

        /*
         * Επιλέγουμε ρητά ποιο callback του δικού μας
         * προγράμματος θα χειριστεί τη σύνδεση.
         */
        ccinfo.local_protocol_name = protocols[0].name;


        if (lws_client_connect_via_info(&ccinfo) == NULL) {
            fprintf(stderr,
                    "Producer: αποτυχία αρχικής σύνδεσης. "
                    "Ξαναδοκιμάζω σε 2 sec.\n");

            lws_context_destroy(context);

            sleep_milliseconds(2000);
            continue;
        }

        printf("Producer: σύνδεση προς Jetstream...\n");


        /*
         * Event loop της libwebsockets.
         * Η lws_service καλεί αυτόματα το websocket_callback
         * όταν συνδεθούμε, λάβουμε μήνυμα ή παρουσιαστεί σφάλμα.
         */
        while (!stop_requested &&
               !state.connection_closed) {

            int result = lws_service(context, 250);

            if (result < 0) {
                fprintf(stderr,
                        "Producer: σφάλμα στη lws_service: %d\n",
                        result);

                state.connection_closed = 1;
            }
        }


        lws_context_destroy(context);

        if (!stop_requested) {
            printf("Producer: reconnect σε 2 sec...\n");

            sleep_milliseconds(2000);
        }
    }


    /*
     * Ενημερώνουμε τον Consumer ότι ο Producer τερμάτισε.
     */
    pthread_mutex_lock(&shared->mutex);

    shared->producer_done = 1;

    pthread_cond_broadcast(&shared->not_empty);

    pthread_mutex_unlock(&shared->mutex);


    printf("Producer τελείωσε.\n");

    return NULL;
}