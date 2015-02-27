#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <error.h>
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

 extern "C" int CBB_read(const char* path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int fd=client._get_fd_from_path(path);
	if(-1 == client._lseek(fd, offset, SEEK_SET))
	{
		return -1;
	}
	return client._read(fd, buffer, size);
}

 extern "C" int CBB_write(const char* path, const void *buffer, size_t size, off_t offset, struct fuse_file_info* fi)
{
	int fd=client._get_fd_from_path(path);
	if(-1 == client._lseek(fd, offset, SEEK_SET))
	{
		return -1;
	}
	return client._write(fd, buffer, size);
}

 extern "C" int CBB_flush(const char* path, struct fuse_file_info* fi)
{
	int fd=client._get_fd_from_path(path);
	return client._flush(fd);
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

 extern "C" int CBB_ftruncate(int fd, off_t length)
{
	CBB_FUNC_P(int, ftruncate, (int fd, off_t length));
	if(CBB::_interpret_fd(fd))
	{
		return client._lseek(fd, length, SEEK_SET);
	}
	else
	{
		MAP_BACK(ftruncate);
		return CBB_REAL(ftruncate)(fd, length);
	}
}

struct fuse_operations CBB_oper;

int main(int argc, char *argv[])
{
	CBB_oper.open=CBB_open;
	CBB_oper.read=CBB_read;
	CBB_oper.write=CBB_write;
	CBB_oper.flush=CBB_flush;
	return fuse_main(argc, argv, &CBB_oper, NULL);
}
