#include <cstring>
#include <unistd.h>
#include <iostream>
#include <ctime>

#include "parsers.h"
#include "utils.h"

struct CpuSample {
    unsigned long idle;
    unsigned long total;
};

struct MemSample {
    unsigned long mem_total_kb;
    unsigned long mem_free_kb;
    unsigned long mem_available_kb;
    unsigned long swap_total_kb;
    unsigned long swap_free_kb;
};

struct LoadSample {
    float minute_load;
    int running_processes;
    int total_processes;
};

// ps = per second
struct NetSample {
    int rx_bytesps;
    int tx_bytesps;
    int rx_packetsps;
    int tx_packetsps;
    int rx_dropsps;
    int tx_dropsps;
};

struct IOSample {
    int read_bytesps;
    int write_bytesps;
    int io_msps;
};

//CPU

float calculate_cpu_util(CpuSample &prev, CpuSample &curr) {
    unsigned long total_delta = curr.total - prev.total;
    unsigned long idle_delta = curr.idle - prev.idle;

    if (total_delta == 0) {
        return 0.0;
    }

    return 100.0 * ( 1.0f - (static_cast<float>(idle_delta) / static_cast<float>(total_delta)));
}

void parse_cpu_sample(int fd, char *buffer, size_t buffer_size, CpuSample &sample) {
    ssize_t n = read_and_copy(fd, buffer, buffer_size);
    if (n <= 0) return;

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

    sample.idle = idle_time;
    sample.total = total;
}

void scan_cpu_usage(char *buffer, size_t buffer_size) {
    static CpuSample prevSample = {};
    static bool has_prev = false;
    static int fd = open_file("/proc/stat");

    CpuSample currSample{};
    parse_cpu_sample(fd, buffer, buffer_size, currSample);

    if (!has_prev) {
        prevSample = currSample;
        has_prev = true;
        return;
    }

    float usage = calculate_cpu_util(prevSample, currSample);

    prevSample = currSample;
}

//MEMORY

bool parse_mem_field(const char *&p, const char *end, const char *key, size_t key_len, unsigned long *output) {
    if (static_cast<size_t>(end - p) >= key_len && std::memcmp(p, key, key_len) == 0) {
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

void parse_mem(int fd, char *buffer, size_t buffer_size, MemSample &sample) {
    ssize_t n = read_and_copy(fd, buffer, buffer_size);
    if (n <= 0) return;

    const char *p = buffer;
    const char *end = buffer + n;

    while (p < end) {
        if (parse_mem_field(p, end, "MemTotal:", 9, &sample.mem_total_kb)) {}
        else if (parse_mem_field(p, end, "MemFree:", 8, &sample.mem_free_kb)) {}
        else if (parse_mem_field(p, end, "MemAvailable:", 13, &sample.mem_available_kb)) {}
        else if (parse_mem_field(p, end, "SwapTotal:", 10, &sample.swap_total_kb)) {}
        else if (parse_mem_field(p, end, "SwapFree:", 9, &sample.swap_free_kb)) {}

        while (p < end && *p != '\n') {
            ++p;
        }
        if (p < end && *p == '\n') {
            ++p;
        }
    }
}

void scan_mem_usage(char *buffer, size_t buffer_size) {
    MemSample sample = {};
    static int fd = open_file("/proc/meminfo");
    parse_mem(fd, buffer, buffer_size, sample);
}

// LOAD

void parse_loadavg(int fd, char *buffer, size_t buffer_size, LoadSample &sample) {
    ssize_t n = read_and_copy(fd, buffer, buffer_size);
    if (n <= 0) return;
    
    const char *p = buffer;
    const char *end = buffer + n;

    int whole = 0, frac = 0, running = 0, total = 0;
    int scale = 1;

    auto next_field = [&] {
        while (p < end && *p != ' ') ++p;
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
}

void scan_load(char *buffer, size_t buffer_size) {
    static int fd = open_file("/proc/loadavg");
    LoadSample sample = {};
    parse_loadavg(fd, buffer, buffer_size, sample);
}

// NETWORK

struct NetCounters {
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
    unsigned long long rx_packets;
    unsigned long long tx_packets;
    unsigned long long rx_drops;
    unsigned long long tx_drops;
};

void parse_net(int fd, char *buffer, size_t buffer_size, NetCounters &counters) {
    ssize_t n = read_and_copy(fd, buffer, buffer_size);
    if (n <= 0) return;

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
}

void calculate_net_rate(const NetCounters &prev, const NetCounters &curr, float dt, NetSample &sample) {  
    if (dt <= 0.0f) {
        sample = {};
        return;
    }

    auto to_rate = [dt](unsigned long long delta) -> int {
        double rate = static_cast<double>(delta) / static_cast<double>(dt);
        return static_cast<unsigned long long>(rate);
    };

    sample.rx_bytesps   = to_rate(curr.rx_bytes   - prev.rx_bytes);
    sample.tx_bytesps   = to_rate(curr.tx_bytes   - prev.tx_bytes);
    sample.rx_packetsps = to_rate(curr.rx_packets - prev.rx_packets);
    sample.tx_packetsps = to_rate(curr.tx_packets - prev.tx_packets);
    sample.rx_dropsps   = to_rate(curr.rx_drops   - prev.rx_drops);
    sample.tx_dropsps   = to_rate(curr.tx_drops   - prev.tx_drops);
}

void scan_net(char *buffer, size_t buffer_size) {
    static int fd = open_file("/proc/net/dev");
    static NetCounters prev = {};
    static struct timespec prev_ts = {};
    static bool has_prev = false;

    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);

    NetCounters curr = {};
    parse_net(fd, buffer, buffer_size, curr);

    if (!has_prev) {
        prev = curr;
        prev_ts = ts;
        has_prev = true;
        return;
    }

    float dt = static_cast<float>(ts.tv_sec - prev_ts.tv_sec) + static_cast<float>(ts.tv_nsec - prev_ts.tv_nsec) / 1e9f;

    NetSample sample = {};
    calculate_net_rate(prev, curr, dt, sample);

    prev = curr;
    prev_ts = ts;

    std::cout << sample.rx_bytesps << "\n";
}

// I/O

struct IOCounters {
    unsigned long long sectors_read;
    unsigned long long sectors_written;
    unsigned long long io_ms;
};

void parse_diskstats(int fd, char *buffer, size_t buffer_size, IOCounters &counters) {
    ssize_t n = read_and_copy(fd, buffer, buffer_size);
    if (n <= 0) return;

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
}

void calculate_io_rate(const IOCounters &prev, const IOCounters &curr, float dt, IOSample &sample) {
    if (dt <= 0.0f) {
        sample = {};
        return;
    }

    auto to_rate = [dt](unsigned long long delta) -> unsigned long long {
        double rate = static_cast<double>(delta) / static_cast<double>(dt);
        return static_cast<unsigned long long>(rate);
    };

    sample.read_bytesps  = to_rate((curr.sectors_read - prev.sectors_read) * 512);
    sample.write_bytesps = to_rate((curr.sectors_written - prev.sectors_written) * 512);
    sample.io_msps       = to_rate(curr.io_ms - prev.io_ms);
}

void scan_io(char *buffer, size_t buffer_size) {
    static int fd = open_file("/proc/diskstats");
    static IOCounters prev = {};
    static struct timespec prev_ts = {};
    static bool has_prev = false;

    struct timespec ts = {};
    clock_gettime(CLOCK_MONOTONIC, &ts);

    IOCounters curr = {};
    parse_diskstats(fd, buffer, buffer_size, curr);

    if (!has_prev) {
        prev = curr;
        prev_ts = ts;
        has_prev = true;
        return;
    }

    float dt = static_cast<float>(ts.tv_sec - prev_ts.tv_sec) + static_cast<float>(ts.tv_nsec - prev_ts.tv_nsec) / 1e9f;

    IOSample sample = {};
    calculate_io_rate(prev, curr, dt, sample);

    prev = curr;
    prev_ts = ts;

    std::cout << sample.read_bytesps << "\n";
}