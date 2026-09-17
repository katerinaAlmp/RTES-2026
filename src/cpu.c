#include <stdio.h>
#include "cpu.h"

int read_cpu_times(CpuTimes *times)
{
    FILE *file = fopen("/proc/stat", "r");

    if (file == NULL) {
        return -1;
    }

    char cpu_label[10];

    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;

    int result = fscanf(file,
                        "%9s %llu %llu %llu %llu %llu %llu %llu %llu",
                        cpu_label,
                        &user,
                        &nice,
                        &system,
                        &idle,
                        &iowait,
                        &irq,
                        &softirq,
                        &steal);

    fclose(file);

    if (result != 9) {
        return -1;
    }

    times->idle = idle + iowait;
    times->total = user + nice + system + idle + iowait + irq + softirq + steal;

    return 0;
}

double calculate_cpu_percent(CpuTimes previous, CpuTimes current)
{
    unsigned long long total_delta = current.total - previous.total;
    unsigned long long idle_delta = current.idle - previous.idle;

    if (total_delta == 0) {
        return 0.0;
    }

    return 100.0 * (1.0 - ((double)idle_delta / (double)total_delta));
}