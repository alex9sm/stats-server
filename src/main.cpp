// g++ -std=c++20 -Wall main.cpp parse_cpu.cpp utils.cpp -o ../main

#include "parse_cpu.h"

#include <iostream>

constexpr size_t buffer_size = 512;
char buffer[buffer_size];

int main() {
    scan_cpu_usage(buffer, buffer_size);
    buffer[0] = '\0';

    return 0;
}