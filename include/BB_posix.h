#ifndef _BB_POSIX_H_

#define _BB_POSIX_H_


extern "C" 
{
	int open(const char *path, int flag, ...); 

	ssize_t read(int fd, void *buffer, size_t size);

	ssize_t write(int fd, const void *buffer, size_t size);

	int close(int fd);

	int flush(int fd);

}

#endif
