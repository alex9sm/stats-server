#pragma once

#include <cstddef>
#include <unistd.h>

int open_file(const char *filepath);
ssize_t read_and_copy(int fd, char *buffer, size_t buffer_size);