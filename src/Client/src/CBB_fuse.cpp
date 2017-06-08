/*
 * Copyright (c) 2017, Tokyo Institute of Technology
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
 * All rights reserved. 
 * 
 * This file is part of HuronFS.
 * 
 * Please also read the file "LICENSE" included in this package for 
 * Our Notice and GNU Lesser General Public License.
 * 
 * This program is free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License (as published by the Free Software 
 * Foundation) version 2.1 dated February 1999. 
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

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

//#define SINGLE_THREAD

using namespace CBB::Common;
using namespace CBB::Client;

extern CBB_stream client;
struct fuse_operations CBB_oper;
extern char* mount_point;

static int CBB_open(const char* path, struct fuse_file_info *fi)
{
	_DEBUG("open with CBB path=%s\n", path);
	mode_t mode=0600;
	int flag=fi->flags;
#ifdef SINGLE_THREAD
	FILE* stream=nullptr;

	start_recording(&client);
	stream=client.open(path, flag, mode);
	end_recording(&client);

#else
	file_meta* stream=nullptr;

	start_recording(&client);
	stream=client.remote_open(path, flag, mode);
	end_recording(&client);
#endif

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

#ifdef SINGLE_THREAD
	start_recording(&client);
	ret=client._flush_stream(stream);
	end_recording(&client);
#endif

	return ret;
}

static int CBB_creat(const char * path, mode_t mode, struct fuse_file_info* fi)
{
	_DEBUG("CBB create file path=%s\n", path);

#ifdef SINGLE_THREAD
	FILE* stream=nullptr;

	start_recording(&client);
	stream=client._open_stream(path, O_CREAT|O_WRONLY|O_TRUNC, mode);
	end_recording(&client);

#else
	file_meta* stream=nullptr;

	start_recording(&client);
	stream=client.remote_open(path, O_CREAT|O_WRONLY|O_TRUNC, mode);
	end_recording(&client);
#endif

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

static int CBB_read(	const char* 	path, 
			char *		buffer,
			size_t 		count, 
			off_t 		offset,
			struct fuse_file_info* fi)
{
#ifdef SINGLE_THREAD
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
#else
	file_meta* stream=(file_meta*)fi->fh;
	int ret=0;
	if(nullptr != stream)
	{

		start_recording(&client);
		ret=client.remote_IO(stream, buffer, offset, count, READ_FILE);
		_DEBUG("ret=%d path=%s\n", ret,path);
		end_recording(&client);

		return ret;
	}
#endif
	else
	{
		errno = EBADF;
		return -1;
	}
}

static int CBB_write(	const char* 	path, 
			const char* 	buffer,
			size_t 		count,
			off_t 		offset,
			struct fuse_file_info* fi)
{
#ifdef SINGLE_THREAD
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
#else
	file_meta* stream=(file_meta*)fi->fh;
	int ret;
	if(nullptr != stream)
	{

		start_recording(&client);
		_DEBUG("path=%s\n", path);
		ret=client.remote_IO(stream, (char*)(buffer), offset, count, WRITE_FILE);
		//client._update_underlying_file_size(stream);
		end_recording(&client);

		return ret;
	}
#endif
	else
	{
		errno = EBADF;
		return -1;
	}
}

static int CBB_getattr(const char* path, struct stat* stbuf)
{
	_DEBUG("CBB getattr path=%s\n", path);

	start_recording(&client);
#ifdef SINGLE_THREAD
	int ret=client.getattr(path, stbuf);
#else
	int ret=client.remote_getattr(path, stbuf);
#endif
	end_recording(&client);

	_DEBUG("ret=%d path=%s file_size=%lu\n", ret,path,stbuf->st_size);
	return ret;
}

static int CBB_readdir(	const char* 	path, 
			void* 		buf, 
			fuse_fill_dir_t filler, 
			off_t 		offset, 
			struct fuse_file_info* fi)
{
	CBB_client::dir_t dir;

	start_recording(&client);
	client.readdir(path,dir);
	end_recording(&client);

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
	start_recording(&client);
	ret=client.unlink(path);
	end_recording(&client);

	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_rmdir(const char* path)
{
	_DEBUG("CBB rmdir path=%s\n", path);

	start_recording(&client);
	int ret=client.rmdir(path);
	end_recording(&client);

	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_access(const char* path, int mode)
{
	_DEBUG("CBB access path=%s\n", path);

	start_recording(&client);
	int ret=client.access(path, mode);
	end_recording(&client);

	_DEBUG("ret=%d path=%s\n", ret, path);
	return ret;
}

static int CBB_release(const char* path, struct fuse_file_info* fi)
{
	_DEBUG("CBB release path=%s\n", path);
#ifdef SINGLE_THREAD
	FILE* stream=(FILE*)fi->fh;
	int ret=-1;

	start_record();
	ret=client._close_stream(stream);
	end_record();
#else
	file_meta* stream=(file_meta*)fi->fh;
	int ret=-1;

	start_recording(&client);
	//ret=client.close(stream);
	end_recording(&client);
#endif
	_DEBUG("ret=%d path=%s\n", ret, path);
	return ret;
}

static int CBB_mkdir(const char* path, mode_t mode)
{
	_DEBUG("CBB mkdir path=%s\n", path);

	start_recording(&client);
	int ret=client.mkdir(path, mode);
	end_recording(&client);

	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_rename(const char* old_name, const char* new_name)
{
	_DEBUG("CBB rename path=%s\n", old_name);

	start_recording(&client);
	int ret=client.rename(old_name, new_name);
	end_recording(&client);

	_DEBUG("ret=%d\n", ret);
	return ret;
}

static int CBB_mknod(const char* path, mode_t mode, dev_t rdev)
{
	_DEBUG("CBB mknod path=%s, mode=%d\n", path, mode);

	//client._open_stream(path, 0600, mode);

	return 0;
}

static int CBB_truncate(const char* path, off_t size)
{
	_DEBUG("CBB truncate path=%s\n", path);

	start_recording(&client);
	int ret=client.truncate(path, size);
	end_recording(&client);

	return ret;
}

static int CBB_ftruncate(const char* path, off_t size, struct fuse_file_info* fi)
{
	_DEBUG("CBB ftruncate path=%s\n", path);

	start_recording(&client);
	int ret=client.truncate(path, size);
	end_recording(&client);

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
	if(!client.initalized())
	{
		printf("client init error\n");
		return EXIT_FAILURE;
	}

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
	
	char** fuse_argv=new char*[argc+4];
#ifdef SINGLE_THREAD
	char* single_thread_string="-s";
#endif
	bool daemon_flag=true;
	
	for(int i=0; i<argc ;++i)
	{
		fuse_argv[i]=argv[i];
		if(0 == strcmp(argv[i], "-d"))
		{
			daemon_flag=false;
		}
	}

#ifdef SINGLE_THREAD
	fuse_argv[argc++]=single_thread_string;
#endif
	fuse_argv[argc++]="-o";
	fuse_argv[argc++]="direct_io";
	fuse_argv[argc++]=mount_point;
	if(!daemon_flag)
	{
		client.start_threads();
	}

	return fuse_main(argc, fuse_argv, &CBB_oper, nullptr);
}
