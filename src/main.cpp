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

int main() {
    
    return 0;
}