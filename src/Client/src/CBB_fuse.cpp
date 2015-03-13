#define FUSE_USE_VERSION 30

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
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
	FILE *ret=NULL;
	_DEBUG("open with CBB path=%s\n", path);
	ret=client._open_stream(path, flag, mode);
	if(NULL == ret)
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
	FILE* stream=client._get_stream_from_path(path);
	if(NULL != stream)
	{
		return client._flush_stream(stream);
	}
	else
	{
		errno = EBADF;
		return -1;
	}
}

static int CBB_creat(const char * path, mode_t mode, struct fuse_file_info* fi)
{
	FILE* ret=NULL;
	_DEBUG("CBB create file path=%s\n", path);
	ret=client._open_stream(path, O_CREAT|O_WRONLY|O_TRUNC, mode);
	if(NULL == ret)
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
	FILE* stream=client._get_stream_from_path(path);
	int ret=0;
	if(NULL != stream)
	{
		if(-1 == client._seek_stream(stream, offset, SEEK_SET))
		{
			return -1;
		}
		ret=client._read_stream(stream, buffer, count);
		_DEBUG("ret=%d, buffer=%s\n", ret, buffer);
		return ret;
	}
	else
	{
		errno = EBADF;
		return -1;
	}
}

static int CBB_write(const char* path, const char*buffer, size_t count, off_t offset, struct fuse_file_info* fi)
{
	FILE* stream=client._get_stream_from_path(path);
	if(NULL != stream)
	{
		if(-1 == client._seek_stream(stream, offset, SEEK_SET))
		{
			return -1;
		}
		return client._write_stream(stream, buffer, count);
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

static int CBB_access(const char* path, int mode)
{
	_DEBUG("CBB access path=%s\n", path);
	int ret=client._access(path, mode);
	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_release(const char* path, struct fuse_file_info* fi)
{
	_DEBUG("CBB access path=%s\n", path);
	FILE* stream=client._get_stream_from_path(path);
	int ret=client._close_stream(stream);
	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_mkdir(const char* path, mode_t mode)
{
	_DEBUG("CBB mkdir path=%s\n", path);
	int ret=client._mkdir(path, mode);
	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_rename(const char* old_name, const char* new_name)
{
	_DEBUG("CBB rename path=%s\n", old_name);
	int ret=client._rename(old_name, new_name);
	_DEBUG("ret=%d\n", ret);
	return ret;
}

int main(int argc, char *argv[])
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
	CBB_oper.access=CBB_access;
	CBB_oper.release=CBB_release;
	CBB_oper.rename=CBB_rename;
	CBB_oper.mkdir=CBB_mkdir;

	return fuse_main(argc, argv, &CBB_oper, NULL);
}
