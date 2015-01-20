#ifndef _BB_POSIX_H_

#define _BB_POSIX_H_

extern const char * mount_point;

extern "C" 
{
	int open(const char *path, int flag, ...); 
	int open64(const char *path, int flag, ...); 
	ssize_t read(int fd, void *buffer, size_t size);
	ssize_t read64(int fd, void *buffer, size_t size);
	ssize_t write(int fd, const void *buffer, size_t size);
	ssize_t write64(int fd, const void *buffer, size_t size);
	int close(int fd);
	int flush(int fd);

	int creat(const char* path, mode_t mode);
	int creat64(const char* path, mode_t mode);
	
	ssize_t readv(int fd, const struct iovec* iov, int iovcnt);
	ssize_t writev(int fd, const struct iovec* iov, int iovcnt);
	ssize_t pread(int fd, void* buf, size_t count, off_t offset);
	ssize_t pread64(int fd, void* buf, size_t count, off64_t offset);
	ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset);
	ssize_t pwrite64(int fd, const void* buf, size_t count, off64_t offset);
	int posix_fadvise(int fd, off_t offset, off_t len, int advice);
	off_t lseek(int fd, off_t offset, int whence);
	off64_t lseek64(int fd, off64_t offset, int whence);

	int ftruncate(int fd, off_t length);
	int fsync(int fd);
	int fdatasync(int fd);
	int flock(int fd, int operation);
	void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
	void* mmap64(void* addr, size_t length, int prot, int flags, int fd, off64_t offset);
	int munmap(void* addr, size_t length);
	int msync(void* addr, size_t length, int flags);



}

extern "C"
{
	int fclose(FILE* stream);
	int fflush(FILE* stream);
	FILE* fopen(const char* path, const char* mode);
	FILE* freopen(const char* path, const char* mode, FILE* stream);
	void setbuf(FILE* stream, char* buf);
	int setvbuf(FILE* stream, char* buf, int type, size_t size);

	int fprintf(FILE* stream, const char* format, ...);
	int fscanf(FILE* stream, const char* format, ...);
	int vfprintf(FILE* stream, const char* format, va_list ap);
	int vfscanf(FILE* stream, const char* format, va_list ap);

	int fgetc(FILE* stream);
	char* fgets(char* s, int n, FILE* stream);
	int fputc(int c, FILE* stream);
	int fputs(const char* s, FILE* stream);
	int getc(FILE* stream);
	int putc(int c, FILE* stream);
	int ungetc(int c, FILE* stream);

	size_t fread(void* ptr, size_t size, size_t nitems, FILE* stream);
	size_t fwrite(const void* ptr, size_t size, size_t nitems, FILE* stream);

	int fgetpos(FILE* stream, fpos_t *pos);
	int fseek(FILE* stream, long offset, int whence);
	int fsetpos(FILE* stream, const fpos_t* pos);
	long ftell(FILE* stream);
	void rewind(FILE* stream);

	void clearerr(FILE* stream);
	int feof(FILE* stream);
	int ferror(FILE* stream);

	int fseeko(FILE* stream, off_t offset, int whence);
	off_t ftello(FILE* stream);
	int fileno(FILE* stream);
}


#endif
