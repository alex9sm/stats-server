// g++ -O3 -std=c++20 -Wall main.cpp parsers.cpp collector.cpp -o ../main

#include <chrono>
#include <cstdio>
#include <thread>

#include "collector.h"

static const char *status_text(int status) {
    switch (status) {
        case SCAN_OK:        return "ok";
        case SCAN_NOT_READY: return "priming";
        default:             return "error";
    }
}

static void print_snapshot(const Snapshot &s) {
    std::printf("---tick---\n");

    if (s.up_status == SCAN_OK) {
        std::printf("uptime: %d\n", s.up.uptime);
    }
}

int main() {
    Collector c;
    int failures = collector_init(c);
    Snapshot s;
    while (true) {
        collector_tick(c, s);
        print_snapshot(s);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    collector_close(c);
    return 0;
}