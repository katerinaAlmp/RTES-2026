#ifndef CPU_H
#define CPU_H

typedef struct {
    unsigned long long total;
    unsigned long long idle;
} CpuTimes;

int read_cpu_times(CpuTimes *times);

double calculate_cpu_percent(CpuTimes previous, CpuTimes current);

#endif