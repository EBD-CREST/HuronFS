#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <error.h>

#include "CBB_stream.h"
//#include "CBB_posix.h"
#include "CBB_internal.h"

using namespace CBB::Common;
using namespace CBB::Client;
CBB_stream client;

/*CBB_FUNC_P(int, fsync, (int fd));
	CBB_FUNC_P(int, fdatasync, (int fd));
	CBB_FUNC_P(int, flock, (int fd, int operation));
	CBB_FUNC_P(void*, mmap, (void *addr, size_t length, int prot, int flag, int fd, off_t offset));
	CBB_FUNC_P(void*, mmap64, (void *addr, size_t length, int prot, int flag, int fd, off64_t offset));
CBB_FUNC_P(int, munmap, (void *addr, size_t length));*/


extern "C" int CBB_WRAP(open)(const char* path, int flag, ...)
{
	va_list ap;
	mode_t mode=0;
	if(flag & O_CREAT)
	{
		va_start(ap, flag);
		mode=va_arg(ap, mode_t);
		va_end(ap);
		return CBB_WRAP(open64)(path, flag, mode);
	}
	else
	{
		return CBB_WRAP(open64)(path, flag);
	}
}

extern "C" int CBB_WRAP(open64)(const char* path, int flag, ...)
{
	va_list ap;
	CBB_FUNC_P(int, open64, (const char *path, int flag, ...));
	mode_t mode=0;
	std::string formatted_path;
	std::string relative_path;
	if(flag & O_CREAT)
	{
		va_start(ap, flag);
		mode=va_arg(ap, mode_t);
		va_end(ap);
	}
	//some problem with path
	//currently path must start with mount point
	CBB_client::_format_path(path, formatted_path);
	if(CBB_client::_interpret_path(formatted_path.c_str()))
	{
		CBB_client::_get_relative_path(formatted_path, relative_path);
		_DEBUG("open with CBB\n");
		return client._open(relative_path.c_str(), flag, mode);
	}
	else
	{
		MAP_BACK(open64);
		if(flag & O_CREAT)
		{
			return CBB_REAL(open64)(path, flag, mode);
		}
		else
		{
			return CBB_REAL(open64)(path, flag);
		}
	}
}

extern "C" ssize_t CBB_WRAP(read)(int fd, void *buffer, size_t size)
{
	CBB_FUNC_P(ssize_t, read, (int fd, void *buffer, size_t size));
	if(CBB_client::_interpret_fd(fd))
	{
		_DEBUG("read from CBB, fd=%d, size=%lu\n", fd, size);
		return client._read(fd, buffer, size);
	}
	else
	{
		MAP_BACK(read);
		return CBB_REAL(read)(fd, buffer, size);
	}
}

extern "C" ssize_t CBB_WRAP(readv)(int fd, const struct iovec *iov, int iovcnt)
{
	CBB_FUNC_P(ssize_t, readv, (int fd, const struct iovec *iov, int iovcnt));
	if(CBB_client::_interpret_fd(fd))
	{
		ssize_t ans=0;
		for(int i=0;i<iovcnt;++i)
		{
			_DEBUG("read from CBB, fd=%d, size=%lu\n", fd, iov[i].iov_len);
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
		return CBB_REAL(readv)(fd, iov, iovcnt);
	}
}

extern "C" ssize_t CBB_WRAP(write)(int fd, const void *buffer, size_t size)
{
	CBB_FUNC_P(ssize_t, write,(int fd, const void*buffer, size_t size));
	if(CBB_client::_interpret_fd(fd))
	{
		_DEBUG("write to CBB, fd=%d, size=%lu\n", fd, size);
		return client._write(fd, buffer, size);
	}
	else
	{
		MAP_BACK(write);
		return CBB_REAL(write)(fd, buffer, size);
	}
}

extern "C" ssize_t CBB_WRAP(writev)(int fd, const struct iovec *iov, int iovcnt)
{
	CBB_FUNC_P(ssize_t, writev, (int fd, const struct iovec *iov, int iovcnt));
	if(CBB_client::_interpret_fd(fd))
	{
		ssize_t ans=0;
		for(int i=0;i<iovcnt;++i)
		{
			_DEBUG("write to CBB, fd=%d, size=%lu\n", fd, iov[i].iov_len);
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
		return CBB_REAL(writev)(fd, iov, iovcnt);
	}
}

extern "C" int CBB_WRAP(close)(int fd)
{
	CBB_FUNC_P(int, close, (int fd));
	if(CBB_client::_interpret_fd(fd))
	{
		_DEBUG("CBB close file fd=%d\n", fd);
		return client._close(fd);
	}
	else
	{
		MAP_BACK(close);
		return CBB_REAL(close)(fd);
	}

}

extern "C" int CBB_WRAP(flush)(int fd)
{
	CBB_FUNC_P(int, flush, (int fd));
	if(CBB_client::_interpret_fd(fd))
	{
		_DEBUG("CBB flush file fd=%d\n", fd);
		return client._flush(fd);
	}
	else
	{
		MAP_BACK(flush);
		return CBB_REAL(flush)(fd);
	}
}

extern "C" off_t CBB_WRAP(lseek)(int fd, off_t offset, int whence)
{
	return CBB_WRAP(lseek64)(fd, offset, whence);
}

extern "C" off64_t CBB_WRAP(lseek64)(int fd, off64_t offset, int whence)
{
	CBB_FUNC_P(off64_t, lseek64, (int fd, off64_t offset, int whence));
	if(CBB_client::_interpret_fd(fd))
	{
		_DEBUG("CBB lseek file fd=%d, offset=%lu\n", fd, offset);
		return client._lseek(fd, offset, whence);
	}
	else
	{
		MAP_BACK(lseek64);
		return CBB_REAL(lseek64)(fd, offset, whence);
	}
}

extern "C" int CBB_WRAP(creat)(const char * path, mode_t mode)
{
	return CBB_WRAP(creat64)(path, mode);
}

extern "C" int CBB_WRAP(creat64)(const char * path, mode_t mode)
{
	CBB_FUNC_P(int, creat64, (const char *path, mode_t mode));
	std::string formatted_path;
	std::string relative_path;
	CBB_client::_format_path(path, formatted_path);
	if(CBB_client::_interpret_path(formatted_path.c_str()))
	{
		CBB_client::_get_relative_path(formatted_path, relative_path);
		_DEBUG("CBB create file path=%s\n", path);
		return client._open(relative_path.c_str(), O_CREAT, mode);
	}
	else
	{
		MAP_BACK(creat64);
		return CBB_REAL(creat64)(path, mode);
	}
}

extern "C" ssize_t CBB_WRAP(pread)(int fd, void *buffer, size_t count, off_t offset)
{
	return CBB_WRAP(pread64)(fd, buffer, count, offset);
}

extern "C" ssize_t CBB_WRAP(pread64)(int fd, void *buffer, size_t count, off64_t offset)
{
	CBB_FUNC_P(ssize_t, pread64, (int fd, void *buffer, size_t count, off64_t offset));
	if(CBB_client::_interpret_fd(fd))
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
		return CBB_REAL(pread64)(fd, buffer, count, offset);
	}
}

extern "C" ssize_t CBB_WRAP(pwrite)(int fd, const void *buffer, size_t count, off_t offset)
{
	return CBB_WRAP(pwrite64)(fd, buffer, count, offset);
}

extern "C" ssize_t CBB_WRAP(pwrite64)(int fd, const void *buffer, size_t count, off64_t offset)
{
	CBB_FUNC_P(ssize_t, pwrite64, (int fd, const void *buffer, size_t count, off64_t offset));
	if(CBB_client::_interpret_fd(fd))
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
		return CBB_REAL(pwrite64)(fd, buffer, count, offset);
	}
}

extern "C" int CBB_WRAP(posix_fadvise)(int fd, off_t offset, off_t len, int advice)
{
	CBB_FUNC_P(int, posix_fadvise, (int fd, off_t offset, off_t len, int advice));
	if(CBB_client::_interpret_fd(fd))
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(posix_fadvise);
		return CBB_REAL(posix_fadvise)(fd, offset, len, advice);
	}
}
extern "C" int CBB_WRAP(ftruncate)(int fd, off_t length)throw()
{
	CBB_FUNC_P(int, ftruncate, (int fd, off_t length));
	if(CBB_client::_interpret_fd(fd))
	{
		return client._lseek(fd, length, SEEK_SET);
	}
	else
	{
		MAP_BACK(ftruncate);
		return CBB_REAL(ftruncate)(fd, length);
	}
}

extern "C" int CBB_WRAP(stat)(const char* path, struct stat* buf)
{
	CBB_FUNC_P(int, stat, (const char* path, struct stat* buf));
	std::string formatted_path;
	std::string relative_path;
	CBB_client::_format_path(path, formatted_path);
	if(CBB_client::_interpret_path(formatted_path.c_str()))
	{
		_DEBUG("CBB stat path=%s\n", path);
		return client._stat(path, buf);
	}
	else
	{
		MAP_BACK(stat);
		return CBB_REAL(stat)(path, buf);
	}
}

/*extern "C" int CBB_WRAP(fstat)(int fd, struct stat* buf)
{
	CBB_FUNC_P(int, fstat, (int fd, struct stat *buf));
	if(CBB_client::_interpret_fd(fd))
	{
		return client._fstat(fd, buf);
	}
	else
	{
		MAP_BACK(fstat);
		return CBB_REAL(fstat)(fd, buf);
	}
}*/

extern "C" int CBB_WRAP(lstat)(const char* path, struct stat* buf)
{
	CBB_FUNC_P(int, lstat, (const char* path, struct stat *buf));
	std::string formatted_path;
	std::string true_path;
	CBB_client::_format_path(path, formatted_path);
	if(CBB_client::_interpret_path(formatted_path.c_str()))
	{
		_DEBUG("do not support yet\n");
		return -1;
	}
	else
	{
		MAP_BACK(lstat);
		return CBB_REAL(lstat)(path, buf);
	}
}
