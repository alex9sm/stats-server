// g++ -O3 -std=c++20 -Wall main.cpp parsers.cpp collector.cpp -o ../main

#include <chrono>
#include <cstdio>
#include <thread>

#include "collector.h"

auto rb = std::make_unique<RingBuffer>();

int main() {
    Collector c;
    int failures = collector_init(c);
    Snapshot s;
    while (true) {
        collector_tick(c, s);
        ring_push(*rb, s);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    collector_close(c);
    return 0;
}