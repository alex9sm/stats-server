// g++ -O3 -std=c++20 -Wall main.cpp parsers.cpp utils.cpp -o ../main

#include <iostream>
#include <chrono>
#include <thread>

#include "parsers.h"

constexpr size_t buffer_size = 512;
char buffer[buffer_size];

int main() {
    scan_load(buffer, buffer_size);

    return 0;
}