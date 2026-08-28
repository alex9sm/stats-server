#include <iostream>
#include <unistd.h>

#include "utils.h"
#include "parse_cpu.h"

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

void parse_cpu_sample(int fd, char *buffer, size_t buffer_size, CpuSample &sample) {

    ssize_t n = read_and_copy(fd, buffer, buffer_size);

    const char *p = buffer + 4;
    const char *end = buffer + n;
    unsigned long idle_time = 0;
    unsigned long total = 0;

    for (int i = 0; i < 8; i++) {
        while (p < end && *p == ' ') {
            ++p;
        }
        unsigned long value = 0;
        while (p < end && *p >= '0' && *p <= '9') {
            value = value * 10 + static_cast<unsigned long>(*p - '0');
            ++p;
        }
        if (i == 3 || i == 4) {
            idle_time += value;
        }
        total += value;
    }

    sample.idle = idle_time;
    sample.total = total;
}

void scan_cpu_usage(char *buffer, size_t buffer_size) {
    static CpuSample prevSample = {};
    static bool has_prev = false;
    static int fd = open_file("/proc/stat");

    CpuSample currSample{};
    parse_cpu_sample(fd, buffer, buffer_size, currSample);

    if (!has_prev) {
        prevSample = currSample;
        has_prev = true;
        return;
    }

    float usage = calculate_util(prevSample, currSample);
    std::cout << usage << "%\n";

    prevSample = currSample;
}