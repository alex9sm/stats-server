// g++ -std=c++20 -Wall main.cpp parse_cpu.cpp parse_mem.cpp utils.cpp -o ../main

#include <iostream>
#include <chrono>
#include <thread>

#include "parse_cpu.h"
#include "parse_mem.h"

constexpr size_t buffer_size = 512;
char buffer[buffer_size];

int main() {
    scan_mem_usage(buffer, buffer_size);

    return 0;
}