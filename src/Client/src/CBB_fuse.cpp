
#define FUSE_USE_VERSION 30

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <error.h>
#include <errno.h>
#include <fuse.h>

#include "CBB_stream.h"
#include "CBB_internal.h"

CBB_stream client;

extern "C" int CBB_open(const char* path, struct fuse_file_info *fi)
{
	mode_t mode=0600;
	int flag=fi->flags;
	std::string formatted_path;
	std::string true_path;
	CBB::_format_path(path, formatted_path);
	CBB::_get_true_path(formatted_path, true_path);
	_DEBUG("open with CBB\n");
	return client._open(true_path.c_str(), flag, mode);
}

extern "C" int CBB_flush(const char *path, struct fuse_file_info* fi)
{
	int fd=client._get_fd_from_path(path);
	if(-1 != fd)
	{
		return client._flush(fd);
	}
	else
	{
		errno = EBADF;
		return -1;
	}
}

extern "C" int CBB_creat(const char * path, mode_t mode, struct fuse_file_info* fi)
{
	std::string formatted_path;
	std::string true_path;
	CBB::_format_path(path, formatted_path);
	CBB::_get_true_path(formatted_path, true_path);
	_DEBUG("CBB create file path=%s\n", path);
	return client._open(true_path.c_str(), O_CREAT, mode);
}

extern "C" int CBB_read(const char* path, char *buffer, size_t count, off_t offset, struct fuse_file_info* fi)
{
	int fd=client._get_fd_from_path(path);
	if(-1 != fd)
	{
		if(-1 == client._lseek(fd, offset, SEEK_SET))
		{
			return -1;
		}
		return client._read(fd, buffer, count);
	}
	else
	{
		errno = EBADF;
		return -1;
	}
}

extern "C" int CBB_write(const char* path, const char*buffer, size_t count, off_t offset, struct fuse_file_info* fi)
{
	int fd=client._get_fd_from_path(path);
	if(-1 != fd)
	{
		if(-1 == client._lseek(fd, offset, SEEK_SET))
		{
			return -1;
		}
		return client._write(fd, buffer, count);
	}
	else
	{
		errno = EBADF;
		return -1;
	}
}


int main(int argc, char **argv)
{
	struct fuse_operations CBB_oper;
	CBB_oper.open=CBB_open;
	CBB_oper.read=CBB_read;
	CBB_oper.write=CBB_write;
	CBB_oper.flush=CBB_flush;
	CBB_oper.create=CBB_creat;
	return fuse_main(argc, argv, &CBB_oper, NULL);
}
