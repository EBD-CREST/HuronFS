#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <error.h>

#include "include/BB.h"
#include "include/BB_posix.h"
#include "include/BB_internal.h"

CBB client;

BB_FUNC_P(int, open, (const char *path, int flag, ...));
BB_FUNC_P(ssize_t, read, (int fd, void *buffer, size_t size));
BB_FUNC_P(ssize_t, write,(int fd, const void*buffer, size_t size));
BB_FUNC_P(int, flush, (int fd));
BB_FUNC_P(int, close, (int fd));


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
	if('/' == path[0])
	{
		formatted_path = std::string(path);
	}
	else
	{
		formatted_path = std::string(mount_point)+std::string(path);
	}
}

extern "C" int BB_WRAP(open)(const char* path, int flag, ...)
{
	va_list ap;
	mode_t mode=0;
	std::string formatted_path;
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
		return client._open(formatted_path.c_str(), flag, mode);
	}
	else
	{
		MAP_BACK(open);
		if(flag & O_CREAT)
		{
			return BB_REAL(open)(path, flag, mode);
		}
		else
		{
			return BB_REAL(open)(path, flag);
		}
	}
}

extern "C" ssize_t BB_WRAP(read)(int fd, void *buffer, size_t size)
{
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

extern "C" ssize_t BB_WRAP(write)(int fd, const void *buffer, size_t size)
{
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

extern "C" int BB_WRAP(close)(int fd)
{
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
