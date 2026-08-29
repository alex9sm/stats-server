// g++ -std=c++20 -Wall main.cpp parse_cpu.cpp parse_mem.cpp parse_load.cpp utils.cpp -o ../main

#include <iostream>
#include <chrono>
#include <thread>

#include "parse_cpu.h"
#include "parse_mem.h"
#include "parse_load.h"

constexpr size_t buffer_size = 512;
char buffer[buffer_size];

int main() {
    scan_load(buffer, buffer_size);

    return 0;
}