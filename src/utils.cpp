#include "utils.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

int open_file(const char *filepath) {
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        throw std::runtime_error("unable to open file");
    }

    return fd;
}

void read_and_copy(int fd, char *buffer, size_t buffer_size) {
    ssize_t bytes_read = pread(fd, buffer, buffer_size - 1, 0);
    buffer[bytes_read] = '\0';
}