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

#include <linux/limits.h>

#include "CBB_stream.h"
#include "CBB_internal.h"

static CBB_stream client;
struct fuse_operations CBB_oper;

static int CBB_open(const char* path, struct fuse_file_info *fi)
{
	mode_t mode=0600;
	int flag=fi->flags;
	FILE* stream=NULL;
	_DEBUG("open with CBB path=%s\n", path);
#ifdef MULTITHREAD
	flockfile(stdout);
#endif
	stream=client._open_stream(path, flag, mode);
#ifdef MULTITHREAD
	funlockfile(stdout);
#endif
	fi->fh=(uint64_t)stream;
	if(NULL == stream)
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
	FILE* stream=(FILE*)(fi->fh);
	int ret=0;
#ifdef MULTITHREAD
	flockfile(stdout);
#endif
	ret=client._flush_stream(stream);
#ifdef MULTITHREAD
	funlockfile(stdout);
#endif
	return ret;
}

static int CBB_creat(const char * path, mode_t mode, struct fuse_file_info* fi)
{
	FILE* stream=NULL;
	_DEBUG("CBB create file path=%s\n", path);
#ifdef MULTITHREAD 
	flockfile(stdout);
#endif
	stream=client._open_stream(path, O_CREAT|O_WRONLY|O_TRUNC, mode);
#ifdef MULTITHREAD
	funlockfile(stdout);
#endif
	fi->fh=(uint64_t)stream;
	if(NULL == stream)
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
	FILE* stream=(FILE*)fi->fh;
	int ret=0;
	//return 2;
	if(NULL != stream)
	{
#ifdef MULTITHREAD
		flockfile(stdout);
#endif
		if(-1 == client._seek_stream(stream, offset, SEEK_SET))
		{
			return -1;
		}
		ret=client._read_stream(stream, buffer, count);
		_DEBUG("ret=%d path=%s\n", ret,path);
#ifdef MULTITHREAD
		funlockfile(stdout);
#endif
	/*	char real_path[PATH_MAX];
		sprintf(real_path, "/tmp/test/%s", path);
		int fd=open(real_path, O_RDONLY);
		pread(fd, buffer, count, offset);*/
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
	FILE* stream=(FILE*)fi->fh;
	int ret;
	if(NULL != stream)
	{
#ifdef MULTITHREAD
		flockfile(stdout);
#endif
		if(-1 == client._seek_stream(stream, offset, SEEK_SET))
		{
			return -1;
		}
		_DEBUG("path=%s\n", path);
		ret=client._write_stream(stream, buffer, count);
#ifdef MULTITHREAD
		funlockfile(stdout);
#endif
		return ret;
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
#ifdef MULTITHREAD
	flockfile(stdout);
#endif
	int ret=client._getattr(path, stbuf);
#ifdef MULTITHREAD
	funlockfile(stdout);
#endif
	_DEBUG("ret=%d path=%s file_size=%lu\n", ret,path,stbuf->st_size);
	return ret;
}

static int CBB_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{
	CBB::dir_t dir;
#ifdef MULTITHREAD
	flockfile(stdout);
#endif
	client._readdir(path,dir);
#ifdef MULTITHREAD
	funlockfile(stdout);
#endif
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
#ifdef MULTITHREAD
	flockfile(stdout);
#endif
	int ret=client._unlink(path);
#ifdef MULTITHREAD
	funlockfile(stdout);
#endif
	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_rmdir(const char* path)
{
	_DEBUG("CBB rmdir path=%s\n", path);
#ifdef MULTITHREAD
	flockfile(stdout);
#endif
	int ret=client._rmdir(path);
#ifdef MULTITHREAD
	funlockfile(stdout);
#endif
	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_access(const char* path, int mode)
{
	_DEBUG("CBB access path=%s\n", path);
#ifdef MULTITHREAD
	flockfile(stdout);
#endif
	int ret=client._access(path, mode);
#ifdef MULTITHREAD
	funlockfile(stdout);
#endif
	_DEBUG("ret=%d path=%s\n", ret, path);
	return ret;
}

static int CBB_release(const char* path, struct fuse_file_info* fi)
{
	FILE* stream=(FILE*)fi->fh;
	int ret=-1;
	_DEBUG("CBB release path=%s\n", path);
#ifdef MULTITHREAD
	flockfile(stdout);
#endif
	ret=client._close_stream(stream);
	_DEBUG("ret=%d path=%s\n", ret, path);
#ifdef MULTITHREAD
	funlockfile(stdout);
#endif
	return ret;
}

static int CBB_mkdir(const char* path, mode_t mode)
{
	_DEBUG("CBB mkdir path=%s\n", path);
#ifdef MULTITHREAD
	flockfile(stdout);
#endif
	int ret=client._mkdir(path, mode);
#ifdef MULTITHREAD
	funlockfile(stdout);
#endif
	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_rename(const char* old_name, const char* new_name)
{
	_DEBUG("CBB rename path=%s\n", old_name);
#ifdef MULTITHREAD
	flockfile(stdout);
#endif
	int ret=client._rename(old_name, new_name);
#ifdef MULTITHREAD
	funlockfile(stdout);
#endif
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
