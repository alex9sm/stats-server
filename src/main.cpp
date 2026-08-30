// g++ -O3 -std=c++20 -Wall main.cpp parsers.cpp -o ../main

#include <iostream>
#include <chrono>
#include <thread>

#include "parsers.h"

constexpr size_t buffer_size = 8192;
char buffer[buffer_size];

int main() {
    while (true) {
        scan_net(buffer, buffer_size);
        std::this_thread::sleep_for(std::chrono::seconds(6));
    }
    return 0;
}