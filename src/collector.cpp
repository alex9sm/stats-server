#include <ctime>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <limits>
#include <cmath>

#include "collector.h"
#include "utils.h"

bool open_metric(int &fd, const char *path) {
    fd = open(path, O_RDONLY);

    if (fd < 0) {
        std::cerr << "collector cannot open " << path << "\n";
    }
    return fd >= 0;
}

void close_metric(int &fd) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

void collector_init(Collector &collector) {

    open_metric(collector.cpu.fd, "/proc/stat");
    open_metric(collector.mem.fd, "/proc/meminfo");
    open_metric(collector.load.fd, "/proc/loadavg");
    open_metric(collector.net.fd, "/proc/net/dev");
    open_metric(collector.io.fd, "/proc/diskstats");
    open_metric(collector.up.fd, "/proc/uptime");
    open_metric(collector.disk.fd, "/");
}

void collector_tick(Collector &collector, Snapshot &out) {
    timespec now = {};
    clock_gettime(CLOCK_MONOTONIC, &now);

    out = Snapshot{};

    out.cpu_status = scan_cpu_usage(collector.cpu, collector.buffer, collector_buffer_size, out.cpu);
    out.mem_status = scan_mem_usage(collector.mem, collector.buffer, collector_buffer_size, out.mem);
    out.load_status = scan_load(collector.load, collector.buffer, collector_buffer_size, out.load);
    out.net_status = scan_net(collector.net, collector.buffer, collector_buffer_size, now, out.net);
    out.io_status = scan_io(collector.io, collector.buffer, collector_buffer_size, now, out.io);
    out.up_status = scan_uptime(collector.up, collector.buffer, collector_buffer_size, out.up);
    out.disk_status = scan_disk(collector.disk, out.disk);

}

void collector_close(Collector &collector) {
    close_metric(collector.cpu.fd);
    close_metric(collector.mem.fd);
    close_metric(collector.load.fd);
    close_metric(collector.net.fd);
    close_metric(collector.io.fd);
    close_metric(collector.disk.fd);

}

void flatten(const Snapshot &s, std::array<float, metrics_count> &out) {
    out.fill(std::numeric_limits<float>::quiet_NaN());

    if (s.cpu_status == SCAN_OK) {
        out[0] = s.cpu.utilization;
    }

    if (s.mem_status == SCAN_OK) {
        out[1] = s.mem.mem_total_kb;        
        out[2] = s.mem.mem_free_kb;
        out[3] = s.mem.mem_available_kb;
        out[4] = s.mem.swap_total_kb;
        out[5] = s.mem.swap_free_kb;
    }

    if (s.load_status == SCAN_OK) {
        out[6] = s.load.minute_load;
        out[7] = s.load.running_processes;
        out[8] = s.load.total_processes;
    }

    if (s.net_status == SCAN_OK) {
        out[9] = s.net.rx_bytesps;
        out[10] = s.net.tx_bytesps;
        out[11] = s.net.rx_packetsps;
        out[12] = s.net.tx_packetsps;
        out[13] = s.net.rx_dropsps;
        out[14] = s.net.tx_dropsps;
    }

    if (s.io_status == SCAN_OK) {
        out[15] = s.io.read_bytesps;
        out[16] = s.io.write_bytesps;
        out[17] = s.io.io_msps;
    }

    if (s.up_status == SCAN_OK) {
        out[18] = s.up.uptime;
    }

    if (s.disk_status == SCAN_OK) {
        out[19] = s.disk.total_bytes;
        out[20] = s.disk.free_bytes;
        out[21] = s.disk.available_bytes;
        out[22] = s.disk.inodes_total;
        out[23] = s.disk.inodes_free;
    }
}

void ring_push(RingBuffer &buffer, const Snapshot &s) {
    std::array<float, metrics_count> row;
    flatten(s, row);
    timespec wall = {};
    clock_gettime(CLOCK_REALTIME, &wall);
    std::lock_guard<std::mutex> lock(buffer.mut);
    if (buffer.total_written == 0) buffer.epoch_first_push = wall.tv_sec;

    buffer.ring_buffer[buffer.head] = row;
    buffer.head = (buffer.head + 1) % ring_cap;
    if (buffer.recorded_count < ring_cap) ++buffer.recorded_count;

    ++buffer.total_written;
}

QueryResult ring_get(RingBuffer &rb, long long from, long long to, long long step) {
    QueryResult result;
    const long long interval = tick_length_seconds;

    long long stride = 1;
    if (step > 0) {
        long long s = step / (interval * 1000);
        if (s > 1) stride = s;
    }

    const double from_s = static_cast<double>(from) / 1000.0;
    const double to_s = static_cast<double>(to) / 1000.0;

    std::lock_guard<std::mutex> lock(rb.mut);
    if (rb.recorded_count == 0) return result;


    const long long t0 = rb.epoch_first_push;
    const long long oldest = static_cast<long long>(rb.total_written - rb.recorded_count);
    const long long newest = static_cast<long long>(rb.total_written) - 1;
    long long start = static_cast<long long>(std::ceil((from_s - t0) / interval));
    long long end = static_cast<long long>(std::floor((to_s - t0) / interval));

    if (start < oldest) start = oldest;
    if (end > newest) end = newest;
    if (start > end) return result;
    if (stride > 1) start = ((start + stride - 1) / stride) * stride;
    for (long long i = start; i < end; i += stride) {
        const long long slot = i % static_cast<long long>(ring_cap);
        const long long ts_ms = (t0 + i * interval) * 1000LL;
        result.time_ms.push_back(ts_ms);
        result.rows.push_back(rb.ring_buffer[slot]);
    }
    return result;
}