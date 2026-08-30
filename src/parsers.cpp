#include <cstring>
#include <unistd.h>
#include <ctime>

#include "parsers.h"
#include "utils.h"

// parsers for each metric. metric source directories are passed by caller in collector.cpp
// ingests counter structs, state, file descriptor, buffer, buffer size. 
// outputs error code and output struct params

//CPU

float calculate_cpu_util(const CpuCounters &prev, CpuCounters &curr) {
    unsigned long long total_delta = counter_delta(prev.total, curr.total);
    unsigned long long idle_delta = counter_delta(prev.idle, curr.idle);

    if (total_delta == 0) {
        return 0.0;
    }

    return 100.0 * ( 1.0f - (static_cast<float>(idle_delta) / static_cast<float>(total_delta)));
}

int parse_cpu_sample(int fd, char *buffer, size_t buffer_size, CpuCounters &counters) {
    ssize_t n = read_file(fd, buffer, buffer_size);
    if (n <= 0) return SCAN_ERROR;

    const char *p = buffer + 4;
    const char *end = buffer + n;
    unsigned long idle_time = 0;
    unsigned long total = 0;

    for (int i = 0; i < 8; i++) {
        while (p < end && *p == ' ') {
            ++p;
        }
        unsigned long value = 0;
        while (is_digit(p, end)) {
            value = value * 10 + static_cast<unsigned long>(*p - '0');
            ++p;
        }
        if (i == 3 || i == 4) {
            idle_time += value;
        }
        total += value;
    }

    counters.idle = idle_time;
    counters.total = total;
    return SCAN_OK;
}

int scan_cpu_usage(CpuState &state, char *buffer, size_t buffer_size, CpuSample &out) {
    if (state.fd < 0) return SCAN_ERROR;

    CpuCounters curr{};
    if (parse_cpu_sample(state.fd, buffer, buffer_size, curr) != SCAN_OK) return SCAN_ERROR;

    if (!state.has_prev) {
        state.prev = curr;
        state.has_prev = true;
        return SCAN_NOT_READY;
    }

    out.utilization = calculate_cpu_util(state.prev, curr);
    state.prev = curr;
    return SCAN_OK;
}

//MEMORY

bool parse_mem_field(const char *&p, const char *end, const char *key, size_t key_len, unsigned long *output) {
    if (has_prefix(p, static_cast<size_t>(end - p), key, key_len)) {
        p += key_len;

        while (p < end && (*p == ' ' || *p == '\t')) {
            ++p;
        }

        unsigned long value = 0;
        while (is_digit(p, end)) {
            value = value * 10 + static_cast<unsigned long>(*p - '0');
            ++p;
        }
        *output = value;
        return true;
    }
    return false;
}

int parse_mem(int fd, char *buffer, size_t buffer_size, MemSample &sample) {
    ssize_t n = read_file(fd, buffer, buffer_size);
    if (n <= 0) return SCAN_ERROR;

    const char *p = buffer;
    const char *end = buffer + n;

    while (p < end) {
        if (parse_mem_field(p, end, "MemTotal:", 9, &sample.mem_total_kb)) {}
        else if (parse_mem_field(p, end, "MemFree:", 8, &sample.mem_free_kb)) {}
        else if (parse_mem_field(p, end, "MemAvailable:", 13, &sample.mem_available_kb)) {}
        else if (parse_mem_field(p, end, "SwapTotal:", 10, &sample.swap_total_kb)) {}
        else if (parse_mem_field(p, end, "SwapFree:", 9, &sample.swap_free_kb)) {}

        p = find_char(p, end, '\n');
        if (p < end) p++;
    }
    return SCAN_OK;
}

int scan_mem_usage(MemState &state, char *buffer, size_t buffer_size, MemSample &out) {
    if (state.fd < 0) return SCAN_ERROR;
    MemSample sample = {};
    if (parse_mem(state.fd, buffer, buffer_size, sample) != SCAN_OK) return SCAN_ERROR;
    out = sample;
    return SCAN_OK;
}

// LOAD

int parse_loadavg(int fd, char *buffer, size_t buffer_size, LoadSample &sample) {
    ssize_t n = read_file(fd, buffer, buffer_size);
    if (n <= 0) return SCAN_ERROR;
    
    const char *p = buffer;
    const char *end = buffer + n;

    int whole = 0, frac = 0, running = 0, total = 0;
    int scale = 1;

    auto next_field = [&] {
        p = find_char(p, end, ' ');
        while (p < end && *p == ' ') ++p;
    };

    while (is_digit(p, end)) {
        whole = whole * 10 + (*p++ - '0');
    }

    if (p < end && *p == '.') {
        ++p;
        while (is_digit(p, end)) {
            frac = frac * 10 + (*p++ - '0');
            scale *= 10;
        }
    }

    next_field();
    next_field();
    next_field();

    while (is_digit(p, end)) {
        running = running * 10 + (*p++ - '0');
    }

    if (p < end && *p == '/') ++p;

    while (is_digit(p, end)) {
        total = total * 10 + (*p++ - '0');
    }

    sample.minute_load = float(whole) + float(frac) / float(scale);
    sample.running_processes = running;
    sample.total_processes = total;

    return SCAN_OK;
}

int scan_load(LoadState &state, char *buffer, size_t buffer_size, LoadSample &out) {
    if (state.fd < 0) return SCAN_ERROR;
    LoadSample sample = {};
    if (parse_loadavg(state.fd, buffer, buffer_size, sample) != SCAN_OK) return SCAN_ERROR;
    out = sample;
    return SCAN_OK;
}

// NETWORK

int parse_net(int fd, char *buffer, size_t buffer_size, NetCounters &counters) {
    ssize_t n = read_file(fd, buffer, buffer_size);
    if (n <= 0) return SCAN_ERROR;

    const char *p = buffer;
    const char *end = buffer + n;

    for (int i = 0; i < 2; i++) {
        p = find_char(p, end, '\n');
        if (p < end) ++p;
    }

    while (p < end) {
        const char *line_end = find_char(p, end, '\n');
        if (line_end == end) {
            break;
        }

        const char *colon = find_char(p, line_end, ':');
        if (colon == line_end) {
            p = line_end + 1;
            continue;
        }

        const char *name = p;
        while (name < colon && *name == ' ') {
            ++name;
        }

        size_t name_len = static_cast<size_t>(colon - name);

        if (!has_prefix(name, name_len, "nic", 3)) {
            p = line_end + 1;
            continue;
        }

        const char *q = colon + 1;
        unsigned long long values[16] = {};
        for (int i = 0; i < 16; i++) {
            while (q < line_end && *q == ' ') {
                ++q;
            }
            unsigned long long value = 0;
            while (is_digit(q, line_end)) {
                value = value * 10 + static_cast<unsigned long long>(*q - '0');
                ++q;
            }
            values[i] = value;
        }

        
        counters.rx_bytes   += values[0];
        counters.rx_packets += values[1];
        counters.rx_drops   += values[3];
        counters.tx_bytes   += values[8];
        counters.tx_packets += values[9];
        counters.tx_drops   += values[11];

        p = line_end + 1;
    }
    return SCAN_OK;
}

void calculate_net_rate(const NetCounters &prev, const NetCounters &curr, double dt, NetSample &sample) {  

    sample.rx_bytesps   = counter_rate(prev.rx_bytes,   curr.rx_bytes,   dt);
    sample.tx_bytesps   = counter_rate(prev.tx_bytes,   curr.tx_bytes,   dt);
    sample.rx_packetsps = counter_rate(prev.rx_packets, curr.rx_packets, dt);
    sample.tx_packetsps = counter_rate(prev.tx_packets, curr.tx_packets, dt);
    sample.rx_dropsps   = counter_rate(prev.rx_drops,   curr.rx_drops,   dt);
    sample.tx_dropsps   = counter_rate(prev.tx_drops,   curr.tx_drops,   dt);
}

int scan_net(NetState &state, char *buffer, size_t buffer_size, const timespec &now, NetSample &out) {
    if (state.fd < 0) return SCAN_ERROR;

    NetCounters curr = {};
    if (parse_net(state.fd, buffer, buffer_size, curr) != SCAN_OK) return SCAN_ERROR;

    if (!state.has_prev) {
        state.prev = curr;
        state.prev_ts = now;
        state.has_prev = true;
        return SCAN_NOT_READY;
    }

    double dt = elapsed_seconds(state.prev_ts, now);
    calculate_net_rate(state.prev, curr, dt, out);

    state.prev = curr;
    state.prev_ts = now;
    return SCAN_OK;
}

// I/O

int parse_diskstats(int fd, char *buffer, size_t buffer_size, IOCounters &counters) {
    ssize_t n = read_file(fd, buffer, buffer_size);
    if (n <= 0) return SCAN_ERROR;

    const char *p = buffer;
    const char *end = buffer + n;

    auto is_target_disk = [](const char *dev, size_t len) {
        if (has_prefix(dev, len, "nvme", 4)) {
            size_t i = len;
            while (i > 0 && dev[i - 1] >= '0' && dev[i - 1] <= '9') --i;
            return i > 0 && dev[i - 1] != 'p';
        }
        if (has_prefix(dev, len, "sd", 2) || has_prefix(dev, len, "hd", 2)) {
            return dev[len - 1] < '0' || dev[len - 1] > '9';
        }
        return false;
    };

    while (p < end) {
        const char *line_end = find_char(p, end, '\n');

        while (p < line_end && (*p == ' ' || *p == '\t')) {
            ++p;
        }

        for (int i = 0; i < 2; i++) {
            while (is_digit(p, line_end)) ++p;
            while (p < line_end && (*p == ' ' || *p == '\t')) ++p;
        }

        const char *device_name = p;
        while (p < line_end && *p != ' ' && *p != '\t') ++p;
        size_t device_name_len = static_cast<size_t>(p - device_name);

        if (device_name_len > 0 && is_target_disk(device_name, device_name_len)) {
            unsigned long long values[10] = {};
            for (int i = 0; i < 10; i++) {
                while (p < line_end && (*p == ' ' || *p == '\t')) {
                    ++p;
                }
                unsigned long long value = 0;
                while (is_digit(p, line_end)) {
                    value = value * 10 + static_cast<unsigned long long>(*p - '0');
                    ++p;
                }
                values[i] = value;
            }

            counters.sectors_read    += values[2];
            counters.sectors_written += values[6];
            counters.io_ms           += values[9];
        }
        p = (line_end < end) ? line_end + 1 : end;
    }
    return SCAN_OK;
}

void calculate_io_rate(const IOCounters &prev, const IOCounters &curr, double dt, IOSample &sample) {
    sample.read_bytesps  = counter_rate(prev.sectors_read, curr.sectors_read, dt) * 512;
    sample.write_bytesps = counter_rate(prev.sectors_written, curr.sectors_written, dt) * 512;
    sample.io_msps       = counter_rate(prev.io_ms, curr.io_ms, dt);
}

int scan_io(IOState &state, char *buffer, size_t buffer_size, const timespec &now, IOSample &out) {
    if (state.fd < 0) return SCAN_ERROR;

    IOCounters curr = {};
    if (parse_diskstats(state.fd, buffer, buffer_size, curr) != SCAN_OK) return SCAN_ERROR;

    if (!state.has_prev) {
        state.prev = curr;
        state.prev_ts = now;
        state.has_prev = true;
        return SCAN_NOT_READY;
    }

    double dt = elapsed_seconds(state.prev_ts, now);
    calculate_io_rate(state.prev, curr, dt, out);
    
    state.prev = curr;
    state.prev_ts = now;
    return SCAN_OK;
}

// UPTIME

int parse_uptime(int fd, char *buffer, size_t buffer_size, UpSample &sample) {
    ssize_t n = read_file(fd, buffer, buffer_size);
    if (n <= 0) return SCAN_ERROR;

    const char *p = buffer;
    const char *end = buffer + n;

    int value = 0;
    while (is_digit(p, end) && *p != '.') {
        value = value * 10 + static_cast<int>(*p++ - '0');
    }

    sample.uptime = value;

    return SCAN_OK;
}

int scan_uptime(UpState &state, char *buffer, size_t buffer_size, UpSample &out) {
    if (state.fd < 0) return SCAN_ERROR;
    UpSample sample = {};
    if (parse_uptime(state.fd, buffer, buffer_size, sample) != SCAN_OK) return SCAN_ERROR;
    out = sample;
    return SCAN_OK;
}