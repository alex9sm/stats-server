#pragma once

#include <cstddef>

void scan_cpu_usage(char *buffer, size_t buffer_size);
void scan_mem_usage(char *buffer, size_t buffer_size);
void scan_load(char *buffer, size_t buffer_size);
void scan_net(char *buffer, size_t buffer_size);
void scan_io(char *buffer, size_t buffer_size);