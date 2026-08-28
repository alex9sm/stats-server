#pragma once

#include <cstddef>

int open_file(const char *filepath);
void read_and_copy(int fd, char *buffer, size_t buffer_size);