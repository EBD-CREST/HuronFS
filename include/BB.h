#ifndef BB_H_
#define BB_H_

int open(const char ** path, ...);

ssize_t read(int fd, void * buffer, size_t size);

ssize_t write(int fd, const void *buffer, size_t size);

int close(int fd);

int flush(int fd);

#endif
