// g++ -O3 -std=c++20 -Wall main.cpp parsers.cpp collector.cpp server.cpp -o ../main

#include <chrono>
#include <cstdio>
#include <thread>

#include "collector.h"
#include "server.h"

auto rb = std::make_unique<RingBuffer>();

int main() {
    server_init();

    Collector c;
    collector_init(c);
    Snapshot s;
    constexpr auto interval = std::chrono::seconds(tick_length_seconds);
    auto next_tick = std::chrono::steady_clock::now();
    while (true) {
        collector_tick(c, s);
        ring_push(*rb, s);
        next_tick += interval;
        std::this_thread::sleep_until(next_tick);
    }
    collector_close(c);
    return 0;
}