#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

#include "config.h"
#include "shared.h"
#include "cpu.h"
#include "monitor.h"

void *monitor_thread(void *arg)
{
    SharedData *shared = (SharedData *)arg;

    /*
     * a+:
     * - δημιουργεί το αρχείο αν δεν υπάρχει,
     * - συνεχίζει στο τέλος αν το πρόγραμμα επανεκκινηθεί,
     * - δεν διαγράφει τα προηγούμενα δεδομένα.
     */
    FILE *log_file = fopen(LOG_FILE, "a+");

    if (log_file == NULL) {
        perror("Δεν μπόρεσα να ανοίξω το metrics_log.txt");
        return NULL;
    }

    /*
     * Η επικεφαλίδα γράφεται μόνο όταν το αρχείο είναι άδειο.
     */
    fseek(log_file, 0, SEEK_END);

    long log_size = ftell(log_file);

    if (log_size == 0) {
        fprintf(log_file,
                "Seconds,Nanoseconds,Commit_Count,Identity_Count,"
                "Account_Count,Info_Count,Buffer_Occupancy_Pct,CPU_Pct\n");

        fflush(log_file);
    }

    CpuTimes previous_cpu;
    CpuTimes current_cpu;

    if (read_cpu_times(&previous_cpu) != 0) {
        previous_cpu.total = 0;
        previous_cpu.idle = 0;
    }

    struct timespec next_time;

    clock_gettime(CLOCK_MONOTONIC, &next_time);

    while (!stop_requested) {

        struct timespec timestamp;
        clock_gettime(CLOCK_REALTIME, &timestamp);

        double cpu_percent = 0.0;

        if (read_cpu_times(&current_cpu) == 0) {
            if (previous_cpu.total != 0) {
                cpu_percent =
                    calculate_cpu_percent(previous_cpu, current_cpu);
            }

            previous_cpu = current_cpu;
        }

        /*
         * Παίρνουμε ένα συνεπές snapshot των counters και του buffer.
         */
        pthread_mutex_lock(&shared->mutex);

        int commit = shared->commit_count;
        int identity = shared->identity_count;
        int account = shared->account_count;
        int info = shared->info_count;
        int buffer_count = shared->buffer.count;
        int done = shared->consumer_done;

        shared->commit_count = 0;
        shared->identity_count = 0;
        shared->account_count = 0;
        shared->info_count = 0;

        pthread_mutex_unlock(&shared->mutex);

        double buffer_occupancy_pct =
            100.0 * ((double)buffer_count / (double)BUFFER_SIZE);

        fprintf(log_file,
                "%ld,%ld,%d,%d,%d,%d,%.2f,%.2f\n",
                (long)timestamp.tv_sec,
                timestamp.tv_nsec,
                commit,
                identity,
                account,
                info,
                buffer_occupancy_pct,
                cpu_percent);

        /*
         * Τα δεδομένα προωθούνται άμεσα στο λειτουργικό σύστημα,
         * ώστε να μη μένουν για πολλή ώρα μόνο στο buffer της stdio.
         */
        fflush(log_file);

        /*
         * Δεν κάνουμε printf κάθε δευτερόλεπτο.
         * Τα αποτελέσματα αποθηκεύονται στο metrics_log.txt.
         */

        if (done) {
            break;
        }

	next_time.tv_sec += 1;

	clock_nanosleep(CLOCK_MONOTONIC,
                	TIMER_ABSTIME,
        	        &next_time,
      		          NULL);
    }

    fclose(log_file);

    printf("Monitor τελείωσε.\n");

    return NULL;
}
