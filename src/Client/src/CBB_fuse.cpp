
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

static CBB_stream client;
struct fuse_operations CBB_oper;

static int CBB_open(const char* path, struct fuse_file_info *fi)
{
	mode_t mode=0600;
	int flag=fi->flags;
	int ret;
	_DEBUG("open with CBB path=%s\n", path);
	ret=client._open(path, flag, mode);
	if(-1 == ret)
	{
		return -errno;
	}
	else
	{
		return 0;
	}
}

static int CBB_flush(const char *path, struct fuse_file_info* fi)
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

static int CBB_creat(const char * path, mode_t mode, struct fuse_file_info* fi)
{
	int ret=0;
	_DEBUG("CBB create file path=%s\n", path);
	ret=client._open(path, O_CREAT|O_WRONLY|O_TRUNC, mode);
	if(-1 == ret)
	{
		return -errno;
	}
	else
	{
		return 0;
	}
}

static int CBB_read(const char* path, char *buffer, size_t count, off_t offset, struct fuse_file_info* fi)
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

static int CBB_write(const char* path, const char*buffer, size_t count, off_t offset, struct fuse_file_info* fi)
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

static int CBB_getattr(const char* path, struct stat* stbuf)
{
	_DEBUG("CBB getattr path=%s\n", path);
	int ret=client._getattr(path, stbuf);
	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{
	CBB::dir_t dir;
	client._readdir(path,dir);
	for(CBB::dir_t::const_iterator it=dir.begin();
			it!=dir.end();++it)
	{
		if(1 == filler(buf, it->c_str(), NULL, 0))
		{
			return -1;
		}
	}
	return 0;
}

static int CBB_unlink(const char* path)
{
	_DEBUG("CBB unlink path=%s\n", path);
	int ret=client._unlink(path);
	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_rmdir(const char* path)
{
	_DEBUG("CBB rmdir path=%s\n", path);
	int ret=client._rmdir(path);
	_DEBUG("ret=%d\n", ret);
	return ret;
}

int main(int argc, char **argv)
{
	CBB_oper.open=CBB_open;
	CBB_oper.read=CBB_read;
	CBB_oper.write=CBB_write;
	CBB_oper.flush=CBB_flush;
	CBB_oper.create=CBB_creat;
	CBB_oper.getattr=CBB_getattr;
	CBB_oper.readdir=CBB_readdir;
	CBB_oper.unlink=CBB_unlink;
	CBB_oper.rmdir=CBB_rmdir;
	return fuse_main(argc, argv, &CBB_oper, NULL);
}
