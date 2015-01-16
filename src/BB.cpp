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

#include "include/BB.h"
#include "include/BB_internal.h"
#include "include/Communication.h"


const char* CBB::CLIENT_MOUNT_POINT="CBB_CLIENT_MOUNT_POINT";
const char* CBB::MASTER_IP="CBB_MASTER_IP";

const char *mount_point=NULL;

/*int ::iob_fstat(ssize_t fd, struct file_stat &stat)
{
	int master_socket=Client::_connect_to_server(client_addr, master_addr); 
	Send(master_socket, GET_FILE_META); 
	Send(master_socket, fd); 
	int ret;
	Recv(master_socket, ret);
	if(ret == SUCCESS)
	{
		Recv(master_socket, stat.file_size);
		Recv(master_socket, stat.block_size);
	}
	return ret;
}*/

CBB::CBB():
	_fid_now(0),
	_file_list(_file_list_t()),
	_opened_file(_file_t(MAX_FILE)),
	_master_addr(sockaddr_in()),
	_client_addr(sockaddr_in()),
	_initial(false)
{
	const char *master_ip=NULL;
	debug("start initalizing\n");
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
	debug("initialization finished\n");
	_initial=true;
}

CBB::~CBB()
{
	for(_file_list_t::iterator it=_file_list.begin();
			it!=_file_list.end();++it)
	{
		_close(it->first);
	}
}

CBB::block_info::block_info(std::string ip, off64_t start_point, size_t size):
	ip(ip),
	start_point(start_point),
	size(size)
{}

CBB::block_info::block_info(const char *ip, off64_t start_point, size_t size):
	ip(ip),
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

void CBB::_getblock(int socket, off64_t start_point, size_t size, std::vector<block_info> &block)
{
	char *ip=NULL;
	
	ssize_t file_size=0;
	size_t block_size=0;
	int count=0;
	Recv(socket, file_size);
	Recv(socket, block_size);
	Send(socket, start_point);
	Send(socket, size);
	Recv(socket, count);
	for(int i=0;i<count;++i)
	{
		Recv(socket, start_point);
		Recvv(socket, &ip);
		block.push_back(block_info(ip, start_point, size));
		free(ip);
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
	debug("connect to master\n");
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
		debug("open finished\n");
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
		debug("file no =%ld, fid = %d\n", file.file_no, _BB_fid_to_fd(fid));
		return _BB_fid_to_fd(fid);
	}
	else
	{
		close(master_socket);
		errno = EBADFD;
		return -1;
	}
}

ssize_t CBB::_read_from_IOnode(file_info& file, const _block_list_t& blocks, char* buffer, size_t size)
{
	ssize_t ans=0;
	int ret=0;
	off64_t current_point=file.current_point;
	size_t read_size=size;
	
	for(_block_list_t::const_iterator it=blocks.begin();
			it!=blocks.end();++it)
	{
		struct sockaddr_in IOnode_addr;

		off64_t offset = current_point-it->start_point;
		size_t IO_size = current_point>it->start_point?it->size-IO_size:size; 
		if(read_size<IO_size)
		{
			IO_size=read_size;
		}

		set_server_addr(it->ip, IOnode_addr);
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
				debug("read size=%ld\n", ret_size);
				debug("IOnode ip=%s\n", it->ip.c_str()); 
				ans+=ret_size;
				file.current_point+=ret_size;
				current_point += ret_size;
				read_size -= ret_size;
				debug("now point=%ld\n", file.current_point);
			}
		}
		close(IOnode_socket);
	}
	return ans;
}

ssize_t CBB::_write_to_IOnode(file_info& file, const _block_list_t& blocks, const char* buffer, size_t size)
{
	ssize_t ans=0;
	int ret=0;
	off64_t current_point=file.current_point;
	size_t write_size=size;
	for(_block_list_t::const_iterator it=blocks.begin();
			it!=blocks.end();++it)
	{
		struct sockaddr_in IOnode_addr;

		off64_t offset = current_point-it->start_point;
		size_t IO_size = current_point>it->start_point?it->size-IO_size:size; 
		if(write_size<IO_size)
		{
			IO_size=write_size;
		}

		set_server_addr(it->ip, IOnode_addr);
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
				debug("write size=%ld\n", ret_size);
				debug("IOnode ip=%s\n", it->ip.c_str()); 
				ans+=ret_size;
				file.current_point+=ret_size;
				current_point += ret_size;
				write_size -= ret_size;
				debug("now point=%ld\n", file.current_point);
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
	try
	{
		file_info& file=_file_list.at(fid);
		CHECK_INIT();

		ssize_t file_no=file.file_no;
		int master_socket=Client::_connect_to_server(_client_addr, _master_addr); 
		_block_list_t blocks;
		Send(master_socket, READ_FILE);
		Send(master_socket, file_no);
		Recv(master_socket, ret);
		if(SUCCESS == ret)
		{
			_getblock(master_socket, file.current_point, size, blocks);
			return _read_from_IOnode(file, blocks, static_cast<char *>(buffer), size);
		}
		else
		{
			errno=EBADFD;
			debug("open file first\n");
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
	try
	{
		file_info& file=_file_list.at(fid);
		ssize_t file_no=file.file_no;
		off64_t &current_point=file.current_point;
		CHECK_INIT();

		int master_socket=Client::_connect_to_server(_client_addr, _master_addr); 
		std::vector<block_info> blocks;
		Send(master_socket, WRITE_FILE);
		Send(master_socket, file_no);
		Recv(master_socket, ret);
		if(SUCCESS == ret)
		{
			_getblock(master_socket, current_point, size, blocks);
			return _write_to_IOnode(file, blocks, static_cast<const char *>(buffer), size);
		}
		else
		{
			errno=EBADFD;
			debug("open file first\n");
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
	file_info & file=_file_list[fid];
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

/*
void User_Client::_set_IOnode_addr(const char* ip)throw(std::runtime_error)
{
	if(0  == inet_aton(ip, &IOnode_addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error"); 
	}
}

int User_Client::iob_flush(ssize_t fd)
{
	int master_socket=Client::_connect_to_server(client_addr, master_addr); 
	Send(master_socket, FLUSH_FILE);
	Send(master_socket, fd);
	int ret=SUCCESS;
	Recv(master_socket, ret);
	return ret;
}

int User_Client::iob_close(ssize_t fd)
{
	int master_socket=Client::_connect_to_server(client_addr, master_addr); 
	Send(master_socket, CLOSE_FILE);
	Send(master_socket, fd);
	int ret=SUCCESS;
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		_file_list.erase(fd);
		_opened_file[]
	}
	return ret;
}
*/
