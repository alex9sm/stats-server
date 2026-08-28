#include <fcntl.h>
#include <unistd.h>
#include <stdexcept>

void open_and_copy(const char *filepath, char *buffer, size_t buffer_size) {
    buffer[0] = '\0';

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        throw std::runtime_error("unable to open file");
    }

    ssize_t bytes_read = read(fd, buffer, buffer_size - 1);
    close(fd);

    buffer[bytes_read] = '\0';
}