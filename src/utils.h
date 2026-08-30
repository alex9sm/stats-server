#pragma once

#include <cstddef>
#include <cstring>
#include <ctime>
#include <unistd.h>

inline ssize_t read_file(int fd, char *buffer, size_t buffer_size) {
    ssize_t bytes_read = pread(fd, buffer, buffer_size - 1, 0);
    if (bytes_read < 0 || bytes_read == static_cast<ssize_t>(buffer_size - 1)) {
        bytes_read = 0;
    }
    buffer[bytes_read] = '\0';
    return bytes_read;
}

inline bool is_digit(const char *p, const char *end) {
    return p < end && *p >= '0' && *p <= '9';
}

inline const char *find_char(const char *p, const char *end, char c) {
    while (p < end && *p != c) {
        ++p;
    }
    return p;
}

inline bool has_prefix(const char *str, size_t str_len, const char *prefix, size_t prefix_len) {
    return str_len >= prefix_len && std::memcmp(str, prefix, prefix_len) == 0;
}

inline double elapsed_seconds(const timespec &start, const timespec &stop) {
    return static_cast<double>(stop.tv_sec - start.tv_sec) + static_cast<double>(stop.tv_nsec - start.tv_nsec) / 1e9;
}

inline unsigned long long counter_delta(unsigned long long prev, unsigned long long curr) {
    return (curr >= prev) ? (curr - prev) : 0ULL;
}

inline double counter_rate(unsigned long long prev, unsigned long long curr, double dt) {
    if (dt <= 0.0) return 0.0;
    return static_cast<double>(counter_delta(prev, curr)) / dt;
}