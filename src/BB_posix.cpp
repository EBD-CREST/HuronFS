#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <error.h>
#include <limits.h>

#include "include/BB.h"
#include "include/BB_posix.h"
#include "include/BB_internal.h"

CBB client;

BB_FUNC_P(int, fsync, (int fd));
BB_FUNC_P(int, fdatasync, (int fd));
BB_FUNC_P(int, flock, (int fd, int operation));
BB_FUNC_P(void*, mmap, (void *addr, size_t length, int prot, int flag, int fd, off_t offset));
BB_FUNC_P(void*, mmap64, (void *addr, size_t length, int prot, int flag, int fd, off64_t offset));
BB_FUNC_P(int, munmap, (void *addr, size_t length));


static bool _interpret_path(const char *path)
{
	if(NULL == mount_point)
	{
		return false;
	}
	if(!strncmp(path, mount_point, strlen(mount_point)))
	{
		return true;
	}
	return false;
}

static bool _interpret_fd(int fd)
{
	if(fd<INIT_FD)
	{
		return false;
	}
	return true;
}

static void _format_path(const char *path, std::string &formatted_path)
{
	char *real_path=static_cast<char *>(malloc(PATH_MAX));
	realpath(path, real_path);

	formatted_path=std::string(real_path);
	free(real_path);
	return;
}

static void _get_true_path(const std::string &formatted_path, std::string &true_path)
{
	const char *pointer=formatted_path.c_str()+strlen(mount_point);
	true_path=std::string(pointer);
	return;
}

extern "C" int BB_WRAP(open)(const char* path, int flag, ...)
{
	va_list ap;
	mode_t mode=0;
	if(flag & O_CREAT)
	{
		va_start(ap, flag);
		mode=va_arg(ap, mode_t);
		va_end(ap);
		return BB_WRAP(open64)(path, flag, mode);
	}
	else
	{
		return BB_WRAP(open64)(path, flag);
	}
}

extern "C" int BB_WRAP(open64)(const char* path, int flag, ...)
{
	va_list ap;
	BB_FUNC_P(int, open64, (const char *path, int flag, ...));
	mode_t mode=0;
	std::string formatted_path;
	std::string true_path;
	if(flag & O_CREAT)
	{
		va_start(ap, flag);
		mode=va_arg(ap, mode_t);
		va_end(ap);
	}
	//some problem with path
	//currently path must start with mount point
	_format_path(path, formatted_path);
	if(_interpret_path(formatted_path.c_str()))
	{
		_get_true_path(formatted_path, true_path);
		return client._open(true_path.c_str(), flag, mode);
	}
	else
	{
		MAP_BACK(open64);
		if(flag & O_CREAT)
		{
			return BB_REAL(open64)(path, flag, mode);
		}
		else
		{
			return BB_REAL(open64)(path, flag);
		}
	}
}

extern "C" ssize_t BB_WRAP(read)(int fd, void *buffer, size_t size)
{
	BB_FUNC_P(ssize_t, read, (int fd, void *buffer, size_t size));
	if(_interpret_fd(fd))
	{
		return client._read(fd, buffer, size);
	}
	else
	{
		MAP_BACK(read);
		return BB_REAL(read)(fd, buffer, size);
	}
}

extern "C" ssize_t BB_WRAP(readv)(int fd, const struct iovec *iov, int iovcnt)
{
	BB_FUNC_P(ssize_t, readv, (int fd, const struct iovec *iov, int iovcnt));
	if(_interpret_fd(fd))
	{
		ssize_t ans=0;
		for(int i=0;i<iovcnt;++i)
		{
			ssize_t ret=client._read(fd, iov[i].iov_base, iov[i].iov_len);
			if( -1 == ret)
			{
				return ret;
			}
			else
			{
				ans+=ret;
			}
		}
		return ans;
	}
	else
	{
		MAP_BACK(readv);
		return BB_REAL(readv)(fd, iov, iovcnt);
	}
}

extern "C" ssize_t BB_WRAP(write)(int fd, const void *buffer, size_t size)
{
	BB_FUNC_P(ssize_t, write,(int fd, const void*buffer, size_t size));
	if(_interpret_fd(fd))
	{
		return client._write(fd, buffer, size);
	}
	else
	{
		MAP_BACK(write);
		return BB_REAL(write)(fd, buffer, size);
	}
}

extern "C" ssize_t BB_WRAP(writev)(int fd, const struct iovec *iov, int iovcnt)
{
	BB_FUNC_P(ssize_t, writev, (int fd, const struct iovec *iov, int iovcnt));
	if(_interpret_fd(fd))
	{
		ssize_t ans=0;
		for(int i=0;i<iovcnt;++i)
		{
			ssize_t ret=client._write(fd, iov[i].iov_base, iov[i].iov_len);
			if( -1 == ret)
			{
				return ret;
			}
			else
			{
				ans+=ret;
			}
		}
		return ans;
	}
	else
	{
		MAP_BACK(writev);
		return BB_REAL(writev)(fd, iov, iovcnt);
	}
}

extern "C" int BB_WRAP(close)(int fd)
{
	BB_FUNC_P(int, close, (int fd));
	if(_interpret_fd(fd))
	{
		return client._close(fd);
	}
	else
	{
		MAP_BACK(close);
		return BB_REAL(close)(fd);
	}

}

extern "C" int BB_WRAP(flush)(int fd)
{
	BB_FUNC_P(int, flush, (int fd));
	if(_interpret_fd(fd))
	{
		return client._flush(fd);
	}
	else
	{
		MAP_BACK(flush);
		return BB_REAL(flush)(fd);
	}
}

extern "C" off_t BB_WRAP(lseek)(int fd, off_t offset, int whence)
{
	return BB_WRAP(lseek64)(fd, offset, whence);
}

extern "C" off64_t BB_WRAP(lseek64)(int fd, off64_t offset, int whence)
{
	BB_FUNC_P(off64_t, lseek64, (int fd, off64_t offset, int whence));
	if(_interpret_fd(fd))
	{
		return client._lseek(fd, offset, whence);
	}
	else
	{
		MAP_BACK(lseek64);
		return BB_REAL(lseek64)(fd, offset, whence);
	}
}

extern "C" int BB_WRAP(creat)(const char * path, mode_t mode)
{
	return BB_WRAP(creat64)(path, mode);
}

extern "C" int BB_WRAP(creat64)(const char * path, mode_t mode)
{
	BB_FUNC_P(int, creat64, (const char *path, mode_t mode));
	std::string formatted_path;
	std::string true_path;
	_format_path(path, formatted_path);
	if(_interpret_path(formatted_path.c_str()))
	{
		_get_true_path(formatted_path, true_path);
		return client._open(true_path.c_str(), O_CREAT, mode);
	}
	else
	{
		MAP_BACK(creat64);
		return BB_REAL(creat64)(path, mode);
	}
}

extern "C" ssize_t BB_WRAP(pread)(int fd, void *buffer, size_t count, off_t offset)
{
	return BB_WRAP(pread64)(fd, buffer, count, offset);
}

extern "C" ssize_t BB_WRAP(pread64)(int fd, void *buffer, size_t count, off64_t offset)
{
	BB_FUNC_P(ssize_t, pread64, (int fd, void *buffer, size_t count, off64_t offset));
	if(_interpret_fd(fd))
	{
		if(-1 == client._lseek(fd, offset, SEEK_SET))
		{
			return -1;
		}
		return client._read(fd, buffer, count);
	}
	else
	{
		MAP_BACK(pread64);
		return BB_REAL(pread64)(fd, buffer, count, offset);
	}
}

extern "C" ssize_t BB_WRAP(pwrite)(int fd, const void *buffer, size_t count, off_t offset)
{
	return BB_WRAP(pwrite64)(fd, buffer, count, offset);
}

extern "C" ssize_t BB_WRAP(pwrite64)(int fd, const void *buffer, size_t count, off64_t offset)
{
	BB_FUNC_P(ssize_t, pwrite64, (int fd, const void *buffer, size_t count, off64_t offset));
	if(_interpret_fd(fd))
	{
		if(-1 == client._lseek(fd, offset, SEEK_SET))
		{
			return -1;
		}
		return client._write(fd, buffer, count);
	}
	else
	{
		MAP_BACK(pwrite64);
		return BB_REAL(pwrite64)(fd, buffer, count, offset);
	}
}

extern "C" int BB_WRAP(posix_fadvise)(int fd, off_t offset, off_t len, int advice)
{
	BB_FUNC_P(int, posix_fadvise, (int fd, off_t offset, off_t len, int advice));
	if(_interpret_fd(fd))
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(posix_fadvise);
		return BB_REAL(posix_fadvise)(fd, offset, len, advice);
	}
}
extern "C" int BB_WRAP(ftruncate)(int fd, off_t length)
{
	BB_FUNC_P(int, ftruncate, (int fd, off_t length));
	if(_interpret_fd(fd))
	{
		return client._lseek(fd, length, SEEK_SET);
	}
	else
	{
		MAP_BACK(ftruncate);
		return BB_REAL(ftruncate)(fd, length);
	}
}

extern "C" int BB_WRAP(stat)(const char* path, struct stat *buf)
{
	BB_FUNC_P(int, stat, (const char* path, struct stat* buf));
	std::string formatted_path;
	std::string true_path;
	_format_path(path, formatted_path);
	if(_interpret_path(formatted_path.c_str()))
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(stat);
		return BB_REAL(stat)(path, buf);
	}
}

extern "C" int BB_WRAP(fstat)(int fd, struct stat* buf)
{
	BB_FUNC_P(int, fstat, (int fd, struct stat *buf));
	if(_interpret_fd(fd))
	{
		return client._fstat(fd, buf);
	}
	else
	{
		MAP_BACK(fstat);
		return BB_REAL(fstat)(fd, buf);
	}
}

extern "C" int BB_WRAP(fclose)(FILE *stream)
{
	BB_FUNC_P(int, fclose, (FILE *stream));
	if(true)
	{

		_DEBUG("do not support yet\n");

		return -1;
	}
	else
	{
		MAP_BACK(fclose);
		return BB_REAL(fclose)(stream);
	}
}

extern "C" int BB_WRAP(fflush)(FILE *stream)
{
	BB_FUNC_P(int, fflush, (FILE *stream));
	if(true)
	{

		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(fflush);
		return BB_REAL(fflush)(stream);
	}
}

extern "C" FILE *BB_WRAP(fopen)(const char* path, const char* mode)
{
	BB_FUNC_P(FILE*, fopen, (const char* path, const char *mode));
	if(true)
	{

		_DEBUG("do not support yet\n");
		return NULL;
	}
	else
	{
		MAP_BACK(fopen);
		return BB_REAL(fopen)(path, mode);
	}
}

extern "C" FILE *BB_WRAP(freopen)(const char* path, const char* mode, FILE* stream)
{
	BB_FUNC_P(FILE*, freopen, (const char* path, const char *mode, FILE* stream));
	if(true)
	{

		_DEBUG("do not support yet\n");
		return NULL;
	}
	else
	{
		MAP_BACK(freopen);
		return BB_REAL(freopen)(path, mode, stream);
	}
}

extern "C" void BB_WRAP(setbuf)(FILE * stream, char * buf)
{
	BB_FUNC_P(void*, setbuf, (FILE* stream, char* buf));

	if(true)
	{
		_DEBUG("do not support yet\n");
	}
	else
	{
		MAP_BACK(setbuf);
		BB_REAL(setbuf)(stream, buf);
	}
}

extern "C" int BB_WRAP(setvbuf)(FILE* stream, char* buf, int type, size_t size)
{
	BB_FUNC_P(int, setvbuf, (FILE* stream, char* buf, int type, size_t size));
	if(true)
	{

		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(setvbuf);
		BB_REAL(setvbuf)(stream, buf, type, size);
	}
}

extern "C" int BB_WRAP(fprintf)(FILE *stream, const char * format, ...)
{
	BB_FUNC_P(int, fprintf, (FILE *stream, const char *format, ...));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return EOF;
	}
	else
	{
		MAP_BACK(fprintf);
		return BB_REAL(fprintf)(stream, format);
	}
}

extern "C" int BB_WRAP(fscanf)(FILE *stream, const char * format, ...)
{
	BB_FUNC_P(int, fscanf, (FILE *stream, const char *format, ...));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return EOF;
	}
	else
	{
		MAP_BACK(fscanf);
		return BB_REAL(fscanf)(stream, format);
	}
}

extern "C" int BB_WRAP(vfprintf)(FILE *stream, const char * format,va_list ap) 
{
	BB_FUNC_P(int, vfprintf, (FILE *stream, const char *format, va_list ap)); 
	if(true)
	{

		_DEBUG("do not support yet\n");
		return EOF;
	}
	else
	{
		MAP_BACK(vfprintf);
		return BB_REAL(vfprintf)(stream, format, ap);
	}
}

extern "C" int BB_WRAP(vfscanf)(FILE *stream, const char * format, va_list ap)
{
	BB_FUNC_P(int, vfscanf, (FILE *stream, const char *format, va_list ap));

	if(true)
	{

		_DEBUG("do not support yet\n");
		return EOF;
	}
	else
	{
		MAP_BACK(vfscanf);
		return BB_REAL(vfscanf)(stream, format, ap);
	}
}

extern "C" int BB_WRAP(fgetc)(FILE *stream)
{
	BB_FUNC_P(int, fgetc, (FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return EOF;
	}
	else
	{
		MAP_BACK(fgetc);
		return BB_REAL(fgetc)(stream);
	}
}

extern "C" char *BB_WRAP(fgets)(char* s, int n, FILE* stream)
{
	BB_FUNC_P(char*, fgets, (char* s, int n, FILE* stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return NULL;
	}
	else
	{
		MAP_BACK(fgets);
		return BB_REAL(fgets)(s, n, stream);
	}
}

extern "C" int BB_WRAP(fputc)(int c, FILE* stream)
{
	BB_FUNC_P(int, fputc, (int c, FILE* stram));
	if(true)
	{

		_DEBUG("do not support yet\n");
		return EOF;
	}
	else
	{
		MAP_BACK(fputc);
		return BB_REAL(fputc)(c, stream);
	}
}

extern "C" int BB_WRAP(fputs)(const char* s, FILE *stream)
{
	BB_FUNC_P(int, fputs, (const char* s, FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return EOF;
	}
	else
	{
		MAP_BACK(fputs);
		return BB_REAL(fputs)(s, stream);
	}
}

extern "C" int BB_WRAP(getc)(FILE *stream)
{
	BB_FUNC_P(int, getc, (FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return EOF;
	}
	else
	{
		MAP_BACK(getc);
		return BB_REAL(getc)(stream);
	}
}

extern "C" int BB_WRAP(putc)(int c, FILE *stream)
{
	BB_FUNC_P(int, putc, (int c, FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return EOF;
	}
	else
	{
		MAP_BACK(putc);
		return BB_REAL(putc)(c, stream);
	}
}

extern "C" int BB_WRAP(ungetc)(int c, FILE *stream)
{
	BB_FUNC_P(int, ungetc, (int c, FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return EOF;
	}
	else
	{
		MAP_BACK(ungetc);
		return BB_REAL(ungetc)(c, stream);
	}
}

extern "C" size_t BB_WRAP(fread)(void *ptr, size_t size, size_t nitems, FILE *stream)
{
	BB_FUNC_P(size_t, fread, (void *ptr, size_t size, size_t nitems, FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return 0;
	}
	else
	{
		MAP_BACK(fread);
		return BB_REAL(fread)(ptr, size, nitems, stream);
	}
}

extern "C" size_t BB_WRAP(fwrite)(const void *ptr, size_t size, size_t nitems, FILE *stream)
{
	BB_FUNC_P(size_t, fwrite, (const void *ptr, size_t size, size_t nitems, FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return 0;
	}
	else
	{
		MAP_BACK(fwrite);
		return BB_REAL(fwrite)(ptr, size, nitems, stream);
	}
}

extern "C" int BB_WRAP(fgetpos)(FILE *stream, fpos_t* pos)
{
	BB_FUNC_P(int, fgetpos, (FILE *stream, fpos_t* pos));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(fgetpos);
		return BB_REAL(fgetpos)(stream, pos);
	}
}

extern "C" int BB_WRAP(fseek)(FILE *stream, long offset, int whence)
{
	BB_FUNC_P(int, fseek, (FILE *stream, long offset, int whence));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(fseek);
		return BB_REAL(fseek)(stream, offset, whence);
	}
}

extern "C" int BB_WRAP(fsetpos)(FILE *stream, const fpos_t* pos)
{
	BB_FUNC_P(int, fsetpos, (FILE *stream, const fpos_t* pos));

	if(true)
	{
		_DEBUG("do not support yet\n");

		return -1;
	}
	else
	{
		MAP_BACK(fsetpos);
		return BB_REAL(fsetpos)(stream, pos);
	}
}

extern "C" long BB_WRAP(ftell)(FILE *stream)
{
	BB_FUNC_P(long, ftell, (FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(ftell);
		return BB_REAL(ftell)(stream);
	}
}

extern "C" void BB_WRAP(rewind)(FILE *stream)
{
	BB_FUNC_P(void, rewind, (FILE *stream));
	if(true)
	{
		_DEBUG("do not support yet\n");
	}
	else
	{
		MAP_BACK(rewind);
		BB_REAL(rewind)(stream);
	}
}

extern "C" void BB_WRAP(clearerr)(FILE *stream)
{
	BB_FUNC_P(void, clearerr, (FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
	}
	else
	{
		MAP_BACK(clearerr);
		BB_REAL(clearerr)(stream);
	}
}

extern "C" int BB_WRAP(feof)(FILE *stream)
{
	BB_FUNC_P(int, feof, (FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(feof);
		return BB_REAL(feof)(stream);
	}
}

extern "C" int BB_WRAP(ferror)(FILE *stream)
{
	BB_FUNC_P(int, ferror, (FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(ferror);
		return BB_REAL(ferror)(stream);
	}
}

extern "C" int BB_WRAP(fseeko)(FILE *stream, off_t offset, int whence)
{
	BB_FUNC_P(int, fseeko, (FILE *stream, off_t offset, int whence));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(fseeko);
		return BB_REAL(fseeko)(stream, offset, whence);
	}
}

extern "C" off_t BB_WRAP(ftello)(FILE *stream)
{
	BB_FUNC_P(off_t, ftello, (FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(ftello);
		return BB_REAL(ftello)(stream);
	}
}

extern "C" int BB_WRAP(fileno)(FILE *stream)
{
	BB_FUNC_P(int, fileno, (FILE *stream));

	if(true)
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(fileno);
		return BB_REAL(fileno)(stream);
	}
}

