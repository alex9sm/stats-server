#include <iostream>

#include "utils.h"

struct LoadSample {
    float minute_load;
    int running_processes;
    int total_processes;
};

void parse_loadavg(int fd, char *buffer, size_t buffer_size, LoadSample &sample) {
    ssize_t n = read_and_copy(fd, buffer, buffer_size);

    const char *p = buffer;
    const char *end = buffer + n;

    int whole = 0, frac = 0, running = 0, total = 0;
    int scale = 1;

    auto next_field = [&] {
        while (p < end && *p != ' ') ++p;
        while (p < end && *p == ' ') ++p;
    };

    while (is_digit(p, end)) {
        whole = whole * 10 + (*p++ - '0');
    }

    if (p < end && *p == '.') {
        ++p;
        while (is_digit(p, end)) {
            frac = frac * 10 + (*p++ - '0');
            scale *= 10;
        }
    }

    next_field();
    next_field();
    next_field();

    while (is_digit(p, end)) {
        running = running * 10 + (*p++ - '0');
    }

    if (p < end && *p == '/') ++p;

    while (is_digit(p, end)) {
        total = total * 10 + (*p++ - '0');
    }

    sample.minute_load = float(whole) + float(frac) / float(scale);
    sample.running_processes = running;
    sample.total_processes = total;
}

void scan_load(char *buffer, size_t buffer_size) {
    static int fd = open_file("/proc/loadavg");
    LoadSample sample = {};
    parse_loadavg(fd, buffer, buffer_size, sample);
    std::cout << sample.minute_load;
}