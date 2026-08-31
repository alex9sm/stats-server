#pragma once

#include <cstddef>
#include <array>
#include <mutex>

#include "parsers.h"

constexpr size_t collector_buffer_size = 32768;
constexpr size_t ring_cap = 518400;
//24 metrics across all snapshot structs
enum metrics {
    cpu_util,
    mem_total, mem_free, mem_available, swap_total, swap_free,
    minute_load, running_procs, total_procs,
    rx_bytesps, tx_bytesps, rx_packetsps, tx_packetsps, rx_dropsps, tx_dropsps,
    read_bytesps, write_bytesps, io_msps,
    uptime,
    total_bytes, free_bytes, avail_bytes, total_inodes, free_inodes,

    metrics_count
};

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

struct RingBuffer {
    std::array<float, metrics_count> ring_buffer[ring_cap];
    size_t head = 0;
    std::mutex mut;
};

int collector_init(Collector &collector);
void collector_tick(Collector &collector, Snapshot &out);
void collector_close(Collector &collector);
void flatten(const Snapshot &s, std::array<float, metrics_count> &out);