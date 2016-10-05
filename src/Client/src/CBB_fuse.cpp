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

#include <execinfo.h>

#include <linux/limits.h>

#include "CBB_stream.h"
#include "CBB_internal.h"

using namespace CBB::Common;
using namespace CBB::Client;

static CBB_stream client;
struct fuse_operations CBB_oper;
extern char* mount_point;


#if 1
#define start_record()

#define end_record()	

#define FLUSH_RECORD()

#define raw_record()

#else

#define recording(rec)					\
		do{					\
		client.rec.file_name=__FILE__;			\
		client.rec.function_name=__func__;		\
		client.rec.line_number=__LINE__;		\
		gettimeofday(&client.rec.time, nullptr);	\
		}while(0)	

#define start_record()				\
		do{				\
			recording(st);	\
		}while(0)

#define end_record()				\
		do{				\
			recording(et);	\
			client._print_time();	\
		}while(0)

#define FLUSH_RECORD()				\
		do{				\
			client.flush_record();	        \
		}while(0)
	
#define raw_record()			\
		do{			\
			recording(raw);	\
			client.print_raw_time();\
		}while(0)
#endif

static int CBB_open(const char* path, struct fuse_file_info *fi)
{
	mode_t mode=0600;
	int flag=fi->flags;
	FILE* stream=nullptr;
	_DEBUG("open with CBB path=%s\n", path);

	start_record();
	stream=client._open_stream(path, flag, mode);
	end_record();

	fi->fh=(uint64_t)stream;
	if(nullptr == stream)
	{
		return -errno;
	}
	else {
		return 0;
	}
}

static int CBB_flush(const char *path, struct fuse_file_info* fi)
{
	FILE* stream=(FILE*)(fi->fh);
	int ret=0;

	start_record();
	ret=client._flush_stream(stream);
	end_record();

	return ret;
}

static int CBB_creat(const char * path, mode_t mode, struct fuse_file_info* fi)
{
	FILE* stream=nullptr;
	_DEBUG("CBB create file path=%s\n", path);

	start_record();
	stream=client._open_stream(path, O_CREAT|O_WRONLY|O_TRUNC, mode);
	end_record();

	fi->fh=(uint64_t)stream;
	if(nullptr == stream)
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
	if(nullptr != stream)
	{

		start_record();
		if(-1 == client._seek_stream(stream, offset, SEEK_SET))
		{
			return -1;
		}
		ret=client._read_stream(stream, buffer, count);
		_DEBUG("ret=%d path=%s\n", ret,path);
		end_record();

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
	if(nullptr != stream)
	{

		start_record();
		if(-1 == client._seek_stream(stream, offset, SEEK_SET))
		{

			return -1;
		}
		_DEBUG("path=%s\n", path);
		ret=client._write_stream(stream, buffer, count);
		client._update_underlying_file_size(stream);
		end_record();

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

	raw_record();
	start_record();
	int ret=client.getattr(path, stbuf);
	end_record();
	raw_record();
	FLUSH_RECORD();

	_DEBUG("ret=%d path=%s file_size=%lu\n", ret,path,stbuf->st_size);
	return ret;
}

static int CBB_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi)
{
	CBB_client::dir_t dir;

	start_record();
	client.readdir(path,dir);
	end_record();

	for(CBB_client::dir_t::const_iterator it=dir.begin();
			it!=dir.end();++it)
	{
		if(1 == filler(buf, it->c_str(), nullptr, 0))
		{
			return -1;
		}
	}
	return 0;
}

static int CBB_unlink(const char* path)
{
	_DEBUG("CBB unlink path=%s\n", path);

	int ret=SUCCESS;
	start_record();
	ret=client.unlink(path);
	end_record();

	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_rmdir(const char* path)
{
	_DEBUG("CBB rmdir path=%s\n", path);

	start_record();
	int ret=client.rmdir(path);
	end_record();

	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_access(const char* path, int mode)
{
	_DEBUG("CBB access path=%s\n", path);

	start_record();
	int ret=client.access(path, mode);
	end_record();

	_DEBUG("ret=%d path=%s\n", ret, path);
	return ret;
}

static int CBB_release(const char* path, struct fuse_file_info* fi)
{
	FILE* stream=(FILE*)fi->fh;
	int ret=-1;
	_DEBUG("CBB release path=%s\n", path);

	start_record();
	ret=client._close_stream(stream);
	end_record();

	_DEBUG("ret=%d path=%s\n", ret, path);
	return ret;
}

static int CBB_mkdir(const char* path, mode_t mode)
{
	_DEBUG("CBB mkdir path=%s\n", path);

	start_record();
	int ret=client.mkdir(path, mode);
	end_record();

	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_rename(const char* old_name, const char* new_name)
{
	_DEBUG("CBB rename path=%s\n", old_name);

	start_record();
	int ret=client.rename(old_name, new_name);
	end_record();

	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_mknod(const char* path, mode_t mode, dev_t rdev)
{
	_DEBUG("CBB mknod path=%s, mode=%d\n", path, mode);

	client._open_stream(path, 0600, mode);

	return 0;
}

static int CBB_truncate(const char* path, off_t size)
{
	_DEBUG("CBB truncate path=%s\n", path);

	start_record();
	int ret=client.truncate(path, size);
	end_record();

	return ret;
}

static int CBB_ftruncate(const char* path, off_t size, struct fuse_file_info* fi)
{
	FILE* stream=(FILE*)fi->fh;
	_DEBUG("CBB ftruncate path=%s\n", path);

	start_record();
	int ret=client._truncate_stream(stream, size);
	end_record();

	return ret;
}

/*static int CBB_utimens(const char* path, const struct timespec ts[2])
{
	_DEBUG("CBB ftruncate path=%s\n", path);
	
	int ret=client._utimens(path, size);
	
	return ret;
}*/

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
	CBB_oper.mknod=CBB_mknod;
	CBB_oper.mkdir=CBB_mkdir;
	CBB_oper.truncate=CBB_truncate;
	CBB_oper.ftruncate=CBB_ftruncate;
	//CBB_oper.utimens=CBB_utimens;
	
	char** fuse_argv=new char*[argc+2];
	char* single_thread_string="-s";
	bool daemon_flag=true;
	for(int i=0; i<argc ;++i)
	{
		fuse_argv[i]=argv[i];
		if(0 == strcmp(argv[i], "-d"))
		{
			daemon_flag=false;
		}
	}

	fuse_argv[argc++]=single_thread_string;
	fuse_argv[argc++]=mount_point;
	if(!daemon_flag)
	{
		client.start_threads();
	}

	return fuse_main(argc, fuse_argv, &CBB_oper, nullptr);
}
