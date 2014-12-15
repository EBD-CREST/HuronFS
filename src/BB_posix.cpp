#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool _interpret_path(const char *path)
{
	if(!strncmp(path, _mount_point, strlen(_mount_point)))
	{
		return true;
	}
	else
	{
		return false;
	}
}

static bool _interpret_id(int fd)
{
	if(id<BB_FD_MIN)
	{
		return false;
	}
	else
	{
		return true;
	}
}

extern "C" int BB_WRAP(open)(const char* path, int flag, ...)
{
	va_list ap;
	mode_t mode;
	if(NULL == _mount_point)
	{
		if(NULL == _mount_point = getenv(MOUNT_POINT))
		{
			errno = EINVAL;
			return -1;
		}
	}
	//some problem with path
	//currently path must start with mount point
	if(flag & O_CREAT)
	{
		va_start(ap, flag);
		mode=va_arg(ap, mode_t);
		va_end(ap);
	}
	if(_interpret_path(path))
	{
		int _new_fd=_get_fd();
		_file_list[_new_fd-]=new file_info;
		return _open(path, flag, mode, _file_list[_new_fd]);
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
	if(NULL == _file_list[fd])
	{
		MAP_BACK(read);
		return orig_read(fd, buffer, size);
	}
	else
	{
		return client._read(fd, buffer, size);
	}
}

extern "C" ssize_t write(int fd, void *buffer, size_t size)
{
	if(NULL == _file_list[fd])
	{
		MAP_BACK(write);
		return BB_REAL(write)(fd, buffer, size);
	}
	else
	{
		return client._write(fd, buffer, size);
	}
}

extern "C" ssize_t close(int fd)
{
	orig_close_f_type orig_close= (orig_close_f_type)dlsym(RTLD_NEXT, "close");
	if(NULL == _file_list[fd])
	{
		return orig_close(fd);
	}
	else
	{
		return client._close(fd);
	}
	
}
