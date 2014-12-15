#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <map>
#include <dlfcn.h>
#include <arpa/inet.h>

#include "include/BB.h"
#include "include/BB_internal.h"
#include "include/Communication.h"

BB_FUNC_P(int, open, (const char *path, int flag, ...));
BB_FUNC_P(ssize_t, read, (int fd, void *buffer, size_t size));
BB_FUNC_P(ssize_t, write,(int fd, const void*buffer, size_t size));
BB_FUNC_P(int, flush, (int fd));
BB_FUNC_P(int, close, (int fd));


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
	_client_addr(sockaddr_in())
{
	const char *master_ip=NULL;
	if(NULL == (master_ip=getenv("BB_MASTER_IP")))
	{
		fprintf(stderr, "please set master ip\n");
		return;
	}
	memset(&_master_addr, 0, sizeof(_master_addr));
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
	_client_addr.sin_port = htons(CLIENT_PORT);
	_client_addr.sin_addr.s_addr = htons(INADDR_ANY);
}

CBB::block_info::block_info(std::string ip, off_t start_point, size_t size):
	ip(ip),
	start_point(start_point),
	size(size)
{}

CBB::block_info::block_info(const char *ip, off_t start_point, size_t size):
	ip(ip),
	start_point(start_point),
	size(size)
{}

CBB::file_info::file_info(ssize_t file_no, int fd, size_t size, size_t block_size):
	file_no(file_no),
	now_point(0),
	fd(fd),
	size(size),
	block_size(block_size)
{}

CBB::file_info::file_info():
	file_no(0),
	now_point(0),
	fd(0),
	size(0),
	block_size(0)
{}

void CBB::_getblock(int socket, off_t start_point, size_t size, std::vector<block_info> &block)
{
	char *ip=NULL;
	
	ssize_t file_size;
	size_t block_size;
	int count;
	Recv(socket, file_size);
	Recv(socket, block_size);
	Recv(socket, count);
	for(int i=0;i<count;++i)
	{
		Recv(socket, start_point);
		Recvv(socket, &ip);
		off_t end_point=(start_point+block_size <= file_size? start_point+block_size:file_size);
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
			return _BB_fid_to_fd(i);
		}
	}
	for(int i=0;i<_fid_now; ++i)
	{
		if(!_opened_file[i])
		{
			_opened_file[i]=true;
			_fid_now=i;
			return _BB_fid_to_fd(i);
		}
	}
	return -1;
}

int CBB::_open(const char * path, int flag, mode_t mode)
{
	int ret;
	int master_socket=client.Client::_connect_to_server(client._client_addr, client._master_addr); 
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
		int fid;
		if(-1 == (fid=_get_fid()))
		{
			errno = EMFILE;
			return -1; 
		}
		file_info &file=_file_list[fid];
		Recv(master_socket, file.file_no);
		Recv(master_socket, file.size);
		Recv(master_socket, file.block_size);
		close(master_socket);
		return _BB_fid_to_fd(fid);
	}
	else
	{
		close(master_socket);
		errno = EBADFD;
		return -1;
	}
}

ssize_t CBB::_read(int fd, void *buffer, size_t size)
{
	int master_socket=client.Client::_connect_to_server(client._client_addr, client._master_addr); 
	int ret;
	off_t start_point;
	ssize_t ans;
	int fid=_BB_fd_to_fid(fd);
	file_info& file=_file_list[fid];
	ssize_t file_no=file.file_no;
	off_t &now_point=file.now_point;
	
	std::vector<block_info> blocks;
	Send(master_socket, READ_FILE);
	Send(master_socket, file_no);
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		_getblock(master_socket, now_point, size, blocks);
		for(std::vector<block_info>::const_iterator it=blocks.begin();
				it!=blocks.end();++it)
		{
			struct sockaddr_in IOnode_addr;
			set_server_addr(it->ip, IOnode_addr);
			int IOnode_socket=Client::_connect_to_server(_client_addr, IOnode_addr);
			Send(IOnode_socket, READ_FILE);
			Send(IOnode_socket, file_no);
			Send(IOnode_socket, start_point);
			size_t recv_size=size < file.block_size ? size: file.block_size;
			Send(IOnode_socket, recv_size);
			Recv(IOnode_socket, ret);
			if(SUCCESS == ret)
			{
				ssize_t ret_size=Recvv_pre_alloc(IOnode_socket, buffer, recv_size);
				if(-1 == ret_size)
				{
					return -1;
				}
				else
				{
					ans+=ret_size;
				}
			}
		}
		return ans;
	}
	else
	{
		fprintf(stderr, "open file first\n");
		close(master_socket);
		return -1;
	}

}

ssize_t CBB::_write(int fd, const void *buffer, size_t size)
{
	int master_socket=client.Client::_connect_to_server(client._client_addr, client._master_addr); 
	int ret;
	off_t start_point;
	ssize_t ans;
	file_info &file=_file_list[fd];
	ssize_t file_no=file.file_no;
	off_t &now_point=file.now_point;
	
	std::vector<block_info> blocks;
	Send(master_socket, WRITE_FILE);
	Send(master_socket, file_no);
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		_getblock(master_socket, now_point, size, blocks);
		for(std::vector<block_info>::const_iterator it=blocks.begin();
				it!=blocks.end();++it)
		{
			struct sockaddr_in IOnode_addr;
			set_server_addr(it->ip, IOnode_addr);
			int IOnode_socket=Client::_connect_to_server(_client_addr, IOnode_addr);
			Send(IOnode_socket, WRITE_FILE);
			Send(IOnode_socket, file_no);
			Send(IOnode_socket, start_point);
			size_t recv_size=size < file.block_size ? size: file.block_size;
			Send(IOnode_socket, recv_size);
			Recv(IOnode_socket, ret);
			if(SUCCESS == ret)
			{
				ssize_t ret_size=Sendv_pre_alloc(IOnode_socket, reinterpret_cast<const char*>(buffer), recv_size);
				if(-1 == ret_size)
				{
					return -1;
				}
				else
				{
					ans+=ret_size;
				}
			}
		}
		return ans;
	}
	else
	{
		fprintf(stderr, "open file first\n");
		close(master_socket);
		return -1;
	}

}

int CBB::_close(int fd)
{
	int master_socket=Client::_connect_to_server(client._client_addr, client._master_addr);
	int ret;
	int fid=_BB_fd_to_fid(fd);
	Send(master_socket, CLOSE_FILE);
	Send(master_socket, _file_list[fd].file_no);
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		_opened_file[fid]=false;
		_file_list.erase(fid);
	}
	return 0;
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
