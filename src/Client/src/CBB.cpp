#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <map>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <limits.h>

#include "CBB.h"
#include "CBB_internal.h"
#include "CBB_const.h"
#include "Communication.h"

const char* CBB::CLIENT_MOUNT_POINT="CBB_CLIENT_MOUNT_POINT";
const char* CBB::MASTER_IP="CBB_MASTER_IP";

const char *mount_point=NULL;

CBB::CBB():
	_fid_now(0),
	_file_list(_file_list_t()),
	_opened_file(_file_t(MAX_FILE)),
	_master_addr(sockaddr_in()),
	_client_addr(sockaddr_in()),
	_initial(false)
{
	const char *master_ip=NULL;
	_DEBUG("start initalizing\n");
	if(NULL == (master_ip=getenv(MASTER_IP)))
	{
		fprintf(stderr, "please set master ip\n");
		return;
	}
	
	if(NULL == (mount_point=getenv(CLIENT_MOUNT_POINT)))
	{
		fprintf(stderr, "please set mount point\n");
		return;
	}
	memset(&_master_addr, 0, sizeof(_master_addr));
	memset(&_client_addr, 0, sizeof(_client_addr));
	_master_addr.sin_family = AF_INET;
	_master_addr.sin_port = htons(MASTER_PORT);
	for(int i=0;i<MAX_FILE; ++i)
	{
		_opened_file[i]=false;
	}

	if(0 == inet_aton(master_ip, &_master_addr.sin_addr))
	{
		perror("Master IP Address Error");
		return;
	}
	_client_addr.sin_family = AF_INET;
	_client_addr.sin_port = htons(0);
	_client_addr.sin_addr.s_addr = htons(INADDR_ANY);
	_DEBUG("initialization finished\n");
	_initial=true;
}

CBB::~CBB()
{
	for(_file_list_t::iterator it=_file_list.begin();
			it!=_file_list.end();++it)
	{
		int ret=0;
		int master_socket=Client::_connect_to_server(_client_addr, _master_addr);
		Send(master_socket, CLOSE_FILE);
		Send(master_socket, it->second.file_no);
		Recv(master_socket, ret);
	}
}

CBB::block_info::block_info(ssize_t node_id, off64_t start_point, size_t size):
	node_id(node_id),
	start_point(start_point),
	size(size)
{}

CBB::file_info::file_info(ssize_t file_no, int fd, size_t size, size_t block_size, int flag):
	file_no(file_no),
	current_point(0),
	fd(fd),
	size(size),
	block_size(block_size),
	flag(flag)
{}

CBB::file_info::file_info():
	file_no(0),
	current_point(0),
	fd(0),
	size(0),
	block_size(0),
	flag(-1)
{}

void CBB::_getblock(int socket, off64_t start_point, size_t& size, std::vector<block_info> &block, _node_pool_t &node_pool)
{
	char *ip=NULL;
	
	size_t file_size=0;
	int count=0;
	ssize_t node_id=0;

	Recv(socket, file_size);
	Send(socket, start_point);
	Send(socket, size);
	Recv(socket, size);
	Recv(socket, count);
	for(int i=0;i<count;++i)
	{
		Recv(socket,node_id);
		Recvv(socket, &ip);
		node_pool.insert(std::make_pair(node_id, std::string(ip)));
		free(ip);
	}
	Recv(socket, count);
	for(int i=0;i<count;++i)
	{
		Recv(socket, start_point);
		Recv(socket, node_id);
		block.push_back(block_info(node_id, start_point, BLOCK_SIZE));
	}
	close(socket);
}
	

int CBB::_get_fid()
{
	for(int i=_fid_now; i<MAX_FILE; ++i)
	{
		if(!_opened_file[i])
		{
			_opened_file[i]=true;
			_fid_now=i;
			return i;
		}
	}
	for(int i=0;i<_fid_now; ++i)
	{
		if(!_opened_file[i])
		{
			_opened_file[i]=true;
			_fid_now=i;
			return i;
		}
	}
	return -1;
}

int CBB::_open(const char * path, int flag, mode_t mode)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int master_socket=Client::_connect_to_server(_client_addr, _master_addr); 
	Send(master_socket, OPEN_FILE); 
	Sendv(master_socket, path, strlen(path));
	Send(master_socket, flag); 
	if(flag & O_CREAT)
	{
		Send(master_socket, mode);
	}
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		_DEBUG("open finished\n");
		int fid;
		if(-1 == (fid=_get_fid()))
		{
			errno = EMFILE;
			return -1; 
		}
		file_info &file=_file_list[fid];
		file.flag=flag;
		Recv(master_socket, file.file_no);
		Recv(master_socket, file.size);
		Recv(master_socket, file.block_size);
		close(master_socket);
		if(flag & O_APPEND)
		{
			file.current_point=file.size;
		}
		_DEBUG("file no =%ld, fid = %d\n", file.file_no, _BB_fid_to_fd(fid));
		int fd=_BB_fid_to_fd(fid);
		file.fd=fd;
		return fd;
	}
	else
	{
		close(master_socket);
		errno = EBADFD;
		return -1;
	}
}

ssize_t CBB::_read_from_IOnode(file_info& file, const _block_list_t& blocks, const _node_pool_t& node_pool, char* buffer, size_t size)
{
	ssize_t ans=0;
	int ret=0;
	off64_t current_point=file.current_point;
	size_t read_size=size;
	if(0 == size)
	{
		return size;
	}
	
	for(_block_list_t::const_iterator it=blocks.begin();
			it!=blocks.end();++it)
	{
		struct sockaddr_in IOnode_addr;

		off64_t offset = current_point-it->start_point;
		size_t IO_size = BLOCK_SIZE-offset;
		if(read_size<IO_size)
		{
			IO_size=read_size;
		}

		set_server_addr(node_pool.at(it->node_id), IOnode_addr);
		int IOnode_socket=Client::_connect_to_server(_client_addr, IOnode_addr);
		Send(IOnode_socket, READ_FILE);
		Send(IOnode_socket, file.file_no);
		Send(IOnode_socket, it->start_point);
		Send(IOnode_socket, offset);
		Send(IOnode_socket, IO_size);
		Recv(IOnode_socket, ret);
		if(SUCCESS == ret)
		{
			ssize_t ret_size=Recvv_pre_alloc(IOnode_socket, buffer, IO_size);
			if(-1 == ret_size)
			{
				return ans;
			}
			else
			{
				_DEBUG("read size=%ld\n", ret_size);
				_DEBUG("IOnode ip=%s\n", node_pool.at(it->node_id).c_str()); 
				ans+=ret_size;
				file.current_point += ret_size;
				current_point += ret_size;
				read_size -= ret_size;
				buffer+=ret_size;
				_DEBUG("current point=%ld\n", file.current_point);
			}
		}
		close(IOnode_socket);
	}
#ifdef DEBUG
	fwrite(buffer-ans, sizeof(char), ans, stderr); 
	fflush(stderr);
#endif
	return ans;
}

ssize_t CBB::_write_to_IOnode(file_info& file, const _block_list_t& blocks, const _node_pool_t& node_pool, const char* buffer, size_t size)
{
	ssize_t ans=0;
	int ret=0;
	off64_t current_point=file.current_point;
	size_t write_size=size;
	if(0 == size)
	{
		return size;
	}
	for(_block_list_t::const_iterator it=blocks.begin();
			it!=blocks.end();++it)
	{
		struct sockaddr_in IOnode_addr;

		off64_t offset = current_point-it->start_point;
		size_t IO_size = BLOCK_SIZE-offset;
		if(write_size<IO_size)
		{
			IO_size=write_size;
		}

		set_server_addr(node_pool.at(it->node_id), IOnode_addr);
		int IOnode_socket=Client::_connect_to_server(_client_addr, IOnode_addr);
		Send(IOnode_socket, WRITE_FILE);
		Send(IOnode_socket, file.file_no);
		Send(IOnode_socket, it->start_point);
		Send(IOnode_socket, offset);
		Send(IOnode_socket, IO_size);
		Recv(IOnode_socket, ret);
		if(SUCCESS == ret)
		{
			ssize_t ret_size=Sendv_pre_alloc(IOnode_socket, buffer, IO_size);
			if(-1 == ret_size)
			{
				return ans;
			}
			else
			{
				_DEBUG("write size=%ld\n", ret_size);
				_DEBUG("IOnode ip=%s\n", node_pool.at(it->node_id).c_str()); 
				ans+=ret_size;
				file.current_point+=ret_size;
				current_point += ret_size;
				buffer+=ret_size;
				write_size -= ret_size;
				_DEBUG("current point=%ld\n", file.current_point);
			}
		}
		close(IOnode_socket);
	}	
	return ans;
}

ssize_t CBB::_read(int fd, void *buffer, size_t size)
{
	int ret=0;
	int fid=_BB_fd_to_fid(fd);
	if(0 == size)
	{
		return size;
	}
	try
	{
		file_info& file=_file_list.at(fid);
		CHECK_INIT();

		ssize_t file_no=file.file_no;
		int master_socket=Client::_connect_to_server(_client_addr, _master_addr); 
		_block_list_t blocks;
		_node_pool_t node_pool;
		Send(master_socket, READ_FILE);
		Send(master_socket, file_no);
		Recv(master_socket, ret);
		if(SUCCESS == ret)
		{
			_getblock(master_socket, file.current_point, size, blocks, node_pool);
			return _read_from_IOnode(file, blocks, node_pool, static_cast<char *>(buffer), size);
		}
		else
		{
			errno=EBADFD;
			_DEBUG("open file first\n");
			close(master_socket);
			return -1;
		}

	}
	catch(std::out_of_range &e)
	{
		errno = EBADFD;
		return -1;
	}
}

ssize_t CBB::_write(int fd, const void *buffer, size_t size)
{
	int ret=0;
	int fid=_BB_fd_to_fid(fd);
	if(0 == size)
	{
		return size;
	}
	try
	{
		file_info& file=_file_list.at(fid);
		ssize_t file_no=file.file_no;
		off64_t &current_point=file.current_point;
		CHECK_INIT();

		int master_socket=Client::_connect_to_server(_client_addr, _master_addr); 
		_block_list_t blocks;
		_node_pool_t node_pool;

		Send(master_socket, WRITE_FILE);
		Send(master_socket, file_no);
		Recv(master_socket, ret);
		if(SUCCESS == ret)
		{
			_getblock(master_socket, current_point, size, blocks, node_pool);
			return _write_to_IOnode(file, blocks, node_pool,  static_cast<const char *>(buffer), size);
		}
		else
		{
			errno=EBADFD;
			_DEBUG("open file first\n");
			close(master_socket);
			return -1;
		}
	}
	catch(std::out_of_range &e)
	{
		errno = EBADFD;
		return -1;
	}

}

int CBB::_close(int fd)
{
	int ret=0;
	int fid=_BB_fd_to_fid(fd);
	CHECK_INIT();
	int master_socket=Client::_connect_to_server(_client_addr, _master_addr);
	Send(master_socket, CLOSE_FILE);
	Send(master_socket, _file_list[fid].file_no);
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		_opened_file[fid]=false;
		_file_list.erase(fid);
		return 0;
	}
	else
	{
		return -1;
	}
}

int CBB::_flush(int fd)
{
	int ret=0;
	int fid=_BB_fd_to_fid(fd);
	CHECK_INIT();
	int master_socket=Client::_connect_to_server(_client_addr, _master_addr);
	Send(master_socket, FLUSH_FILE);
	Send(master_socket, _file_list[fid].file_no);
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

off64_t CBB::_lseek(int fd, off64_t offset, int whence)
{
	int fid=_BB_fd_to_fid(fd);
	CHECK_INIT();
	try
	{
		file_info & file=_file_list.at(fid);
		switch(whence)
		{
			case SEEK_SET:
				file.current_point=offset;break;
			case SEEK_CUR:
				file.current_point+=offset;break;
			case SEEK_END:
				file.current_point=file.size+offset;break;
		}
		return offset;
	}
	catch(std::out_of_range &e)
	{
		errno=EBADF;
		return -1;
	}
}

int CBB::_fstat(int fd, struct stat* buf)
{
	int fid=_BB_fd_to_fid(fd);
	CHECK_INIT();
	memset(buf, 0, sizeof(struct stat));
	try
	{
		file_info & file=_file_list.at(fid);
		buf->st_size=file.size;
		return 0;
	}
	catch(std::out_of_range &e)
	{
		errno=EBADF;
		return -1;
	}
}

off64_t CBB::_tell(int fd)
{
	int fid=_BB_fd_to_fid(fd);
	CHECK_INIT();
	return _file_list.at(fid).current_point;
}

bool CBB::_interpret_path(const char *path)
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

bool CBB::_interpret_fd(int fd)
{
	if(fd<INIT_FD)
	{
		return false;
	}
	return true;
}

void CBB::_format_path(const char *path, std::string &formatted_path)
{
	char *real_path=static_cast<char *>(malloc(PATH_MAX));
	realpath(path, real_path);

	formatted_path=std::string(real_path);
	free(real_path);
	return;
}

void CBB::_get_true_path(const std::string &formatted_path, std::string &true_path)
{
	const char *pointer=formatted_path.c_str()+strlen(mount_point);
	true_path=std::string(pointer);
	return;
}

int CBB::_BB_fd_to_fid(int fd)
{
	return fd-INIT_FD;
}

int CBB::_BB_fid_to_fd(int fid)
{
	return fid+INIT_FD;
}

size_t CBB::_get_file_size(int fd)
{
	int fid=_BB_fd_to_fid(fd);
	CHECK_INIT();
	return _file_list.at(fid).size;
}
