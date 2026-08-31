#pragma once

#include <cstddef>

#include "parsers.h"

constexpr size_t collector_buffer_size = 8192;

struct Snapshot {
    CpuSample cpu = {};
    MemSample mem = {};
    LoadSample load = {};
    NetSample net = {};
    IOSample io = {};
    UpSample up = {};
    DiskSample disk = {};
    int cpu_status = SCAN_NOT_READY;
    int mem_status = SCAN_NOT_READY;
    int load_status = SCAN_NOT_READY;
    int net_status = SCAN_NOT_READY;
    int io_status = SCAN_NOT_READY;
    int up_status = SCAN_NOT_READY;
    int disk_status = SCAN_NOT_READY;
};

struct Collector {
    CpuState cpu;
    MemState mem;
    LoadState load;
    NetState net;
    IOState io;
    UpState up;
    DiskState disk;

    char buffer[collector_buffer_size];
};

int collector_init(Collector &collector);
void collector_tick(Collector &collector, Snapshot &out);
void collector_close(Collector &collector);