#include <cstring>
#include <iostream>
#include <unistd.h>

#include "utils.h"

struct MemSample {
    unsigned long mem_total_kb;
    unsigned long mem_free_kb;
    unsigned long mem_available_kb;
    unsigned long swap_total_kb;
    unsigned long swap_free_kb;
};

bool parse_mem_field(const char *&p, const char *end, const char *key, size_t key_len, unsigned long *output) {
    if (static_cast<size_t>(end - p) >= key_len && std::memcmp(p, key, key_len) == 0) {
        p += key_len;

        while (p < end && (*p == ' ' || *p == '\t')) {
            ++p;
        }

        unsigned long value = 0;
        while (p < end && *p >= '0' && *p <= '9') {
            value = value * 10 + static_cast<unsigned long>(*p - '0');
            ++p;
        }
        *output = value;
        return true;
    }
    return false;
}

void parse_mem_sample(int fd, char *buffer, size_t buffer_size, MemSample &sample) {
    ssize_t n = read_and_copy(fd, buffer, buffer_size);

    const char *p = buffer;
    const char *end = buffer + n;

    while (p < end) {
        if (parse_mem_field(p, end, "MemTotal:", 9, &sample.mem_total_kb)) {}
        else if (parse_mem_field(p, end, "MemFree:", 8, &sample.mem_free_kb)) {}
        else if (parse_mem_field(p, end, "MemAvailable:", 13, &sample.mem_available_kb)) {}
        else if (parse_mem_field(p, end, "SwapTotal:", 10, &sample.swap_total_kb)) {}
        else if (parse_mem_field(p, end, "SwapFree:", 9, &sample.swap_free_kb)) {}

        while (p < end && *p != '\n') {
            ++p;
        }
        if (p < end && *p == '\n') {
            ++p;
        }
    }
}

void scan_mem_usage(char *buffer, size_t buffer_size) {
    MemSample sample = {};
    static int fd = open_file("/proc/meminfo");
    parse_mem_sample(fd, buffer, buffer_size, sample);
    std::cout << sample.mem_total_kb << " " << sample.mem_available_kb;
}