#include "utils.h"
#include "parse_cpu.h"

#include <iostream>

struct CpuSample {
    int idle;
    int total;
};

float calculate_util(CpuSample &prev, CpuSample &curr) {
    int total_delta = curr.total - prev.total;
    int idle_delta = curr.idle - prev.idle;

    if (total_delta == 0) {
        return 0.0;
    }

    return 100.0 * (static_cast<float>(idle_delta) / static_cast<float>(total_delta));
}

void parse_and_update_sample(char *buffer, size_t buffer_size, CpuSample &sample) {
    open_and_copy("/proc/stat", buffer, buffer_size);
    int parsed = sscanf(buffer, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
                        &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    
    int idle_time = idle + iowait;
    int active_time = user + nice + system + irq + softirq + steal;

    sample.idle = idle_time;
    sample.total = idle_time + active_time;
}

void cpu_util(char *buffer, size_t buffer_size) {
    CpuSample prevSample{};
    parse_and_update_sample(buffer, buffer_size, prevSample);
    
}