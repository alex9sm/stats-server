#include "utils.h"
#include "parse_cpu.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>

struct CpuSample {
    unsigned long idle;
    unsigned long total;
};

float calculate_util(CpuSample &prev, CpuSample &curr) {
    unsigned long total_delta = curr.total - prev.total;
    unsigned long idle_delta = curr.idle - prev.idle;

    if (total_delta == 0) {
        return 0.0;
    }

    return 100.0 * ( 1.0f - (static_cast<float>(idle_delta) / static_cast<float>(total_delta)));
}

void parse_sample(int fd, char *buffer, size_t buffer_size, CpuSample &sample) {
    lseek(fd, 0, SEEK_SET);

    read_and_copy(fd, buffer, buffer_size);

    unsigned long user, nice, system, idle, iowait, irq, softirq, steal; 

    int parsed = sscanf(buffer, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
                        &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    
    unsigned long idle_time = idle + iowait;
    unsigned long active_time = user + nice + system + irq + softirq + steal;

    sample.idle = idle_time;
    sample.total = idle_time + active_time;
}

void scan_cpu_usage(char *buffer, size_t buffer_size) {
    int fd = open_file("/proc/stat");

    CpuSample prevSample{};
    CpuSample currSample{};

    parse_sample(fd, buffer, buffer_size, prevSample);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    parse_sample(fd, buffer, buffer_size, currSample);

    float usage = calculate_util(prevSample, currSample);
    std::cout << usage << "%\n";
}