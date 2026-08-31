#include <ctime>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

#include "collector.h"
#include "utils.h"

bool open_metric(int &fd, const char *path) {
    fd = open(path, O_RDONLY);

    if (fd < 0) {
        std::cerr << "collector cannot open " << path << "\n";
    }
    return true;
}

void close_metric(int &fd) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

int collector_init(Collector &collector) {
    int failures = 0;

    if (!open_metric(collector.cpu.fd, "/proc/stat")) ++failures;
    if (!open_metric(collector.mem.fd, "/proc/meminfo")) ++failures;
    if (!open_metric(collector.load.fd, "/proc/loadavg")) ++failures;
    if (!open_metric(collector.net.fd, "/proc/net/dev")) ++failures;
    if (!open_metric(collector.io.fd, "/proc/diskstats")) ++failures;
    if (!open_metric(collector.up.fd, "/proc/uptime")) ++failures;
    if (!open_metric(collector.disk.fd, "/")) ++ failures;
    return failures;
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