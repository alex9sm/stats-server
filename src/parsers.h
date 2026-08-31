#pragma once

#include <cstddef>

enum ScanStatus {
    SCAN_OK = 0,
    SCAN_NOT_READY = 1,
    SCAN_ERROR = -1,
};

// CPU

struct CpuCounters {
    unsigned long idle;
    unsigned long total;
};

struct CpuSample {
    float utilization;
};

struct CpuState {
    int fd = -1;
    CpuCounters prev = {};
    bool has_prev = false;
};

// MEMORY

struct MemSample {
    unsigned long mem_total_kb;
    unsigned long mem_free_kb;
    unsigned long mem_available_kb;
    unsigned long swap_total_kb;
    unsigned long swap_free_kb;
};

struct MemState {
    int fd = -1;
};

// LOAD

struct LoadSample {
    float minute_load;
    int running_processes;
    int total_processes;
};

struct LoadState {
    int fd = -1;
};

// NETWORK

struct NetCounters {
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    unsigned long long rx_packets;
    unsigned long long tx_packets;
    unsigned long long rx_drops;
    unsigned long long tx_drops;
};

// ps = per second
struct NetSample {
    double rx_bytesps;
    double tx_bytesps;
    double rx_packetsps;
    double tx_packetsps;
    double rx_dropsps;
    double tx_dropsps;
};

struct NetState {
    int fd = -1;
    NetCounters prev = {};
    timespec prev_ts = {};
    bool has_prev = false;
};

// I/O

struct IOCounters {
    unsigned long long sectors_read;
    unsigned long long sectors_written;
    unsigned long long io_ms;
};

struct IOSample {
    double read_bytesps;
    double write_bytesps;
    double io_msps;
};

struct IOState {
    int fd = -1;
    IOCounters prev = {};
    timespec prev_ts = {};
    bool has_prev = false;
};

// UPTIME

struct UpSample {
    int uptime;
};

struct UpState {
    int fd = -1;
};

// FILE SYSTEM

struct DiskSample {
    unsigned long long total_bytes;
    unsigned long long free_bytes;
    unsigned long long available_bytes;  
    unsigned long long inodes_total;
    unsigned long long inodes_free;
};

struct DiskState {
    int fd = -1;
};

int scan_cpu_usage(CpuState &state, char *buffer, size_t buffer_size, CpuSample &out);
int scan_mem_usage(MemState &state, char *buffer, size_t buffer_size, MemSample &out);
int scan_load(LoadState &state, char *buffer, size_t buffer_size, LoadSample &out);
int scan_net(NetState &state, char *buffer, size_t buffer_size, const timespec &now, NetSample &out);
int scan_io(IOState &state, char *buffer, size_t buffer_size, const timespec &now, IOSample &out);
int scan_uptime(UpState &state, char *buffer, size_t buffer_size, UpSample &out);
int scan_disk(DiskState &state, DiskSample &out);