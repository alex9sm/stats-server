// g++ -std=c++20 -Wall main.cpp parse_cpu.cpp utils.cpp -o ../main

#include "parse_cpu.h"

#include <iostream>

char buffer[512];
size_t buffer_size = sizeof(buffer);

int main() {
    parse_cpu(buffer, buffer_size);
    return 0;
}