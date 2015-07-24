/*
 * IOnode.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/limits.h>

#include "IOnode.h"
#include "CBB_const.h"
#include "Communication.h"

const char *IOnode::IONODE_MOUNT_POINT="CBB_IONODE_MOUNT_POINT";

IOnode::block::block(off64_t start_point, size_t data_size, bool dirty_flag, bool valid) throw(std::bad_alloc):
	data_size(data_size),
	data(NULL),
	start_point(start_point),
	dirty_flag(dirty_flag),
	valid(INVALID)
{
	if(INVALID != valid)
	{
		data=malloc(BLOCK_SIZE);
		if( NULL == data)
		{
		   throw std::bad_alloc();
		}
	}
	return;
}

IOnode::block::~block()
{
	free(data);
}

IOnode::block::block(const block & src):
	data_size(src.data_size),
	data(src.data), 
	start_point(src.start_point),
	dirty_flag(src.dirty_flag),
	valid(src.valid)
{};

void IOnode::block::allocate_memory()throw(std::bad_alloc)
{
	if(NULL != data)
	{
		free(data);
	}
	data=malloc(BLOCK_SIZE);
	if(NULL == data)
	{
		throw std::bad_alloc();
	}
	return;
}

IOnode::IOnode(const std::string& master_ip,
		int master_port) throw(std::runtime_error):
	Server(IONODE_PORT), 
	Client(), 
	_node_id(-1),
	_files(file_blocks_t()),
	_file_path(file_path_t()),
	_current_block_number(0), 
	_MAX_BLOCK_NUMBER(MAX_BLOCK_NUMBER), 
	_memory(MEMORY), 
	_master_port(MASTER_PORT),
	_mount_point(std::string()),
	_master_socket(-1)
{
	memset(&_master_conn_addr, 0, sizeof(_master_conn_addr));
	memset(&_master_addr, 0, sizeof(_master_addr));
	if(-1  ==  (_node_id=_regist(master_ip,  master_port)))
	{
		throw std::runtime_error("Get Node Id Error"); 
	}
	const char *IOnode_mount_point=getenv(IONODE_MOUNT_POINT);
	if( NULL == IOnode_mount_point)
	{
		throw std::runtime_error("please set IONODE_MOUNT_POINT environment value");
	}
	_mount_point=std::string(IOnode_mount_point);
	try
	{
		Server::_init_server();
	}
	catch(std::runtime_error& e)
	{
		_unregist();
		throw;
	}
}

IOnode::~IOnode()
{
	_DEBUG("I am going to shut down\n");
	_unregist();
}

int IOnode::_unregist()
{
	Send_flush(_master_socket, CLOSE_CLIENT);
	close(_master_socket);
	return SUCCESS;
}

ssize_t IOnode::_regist(const std::string& master_ip,
		int master_port) throw(std::runtime_error)
{ 
	_master_conn_addr.sin_family = AF_INET;
	_master_conn_addr.sin_addr.s_addr = htons(INADDR_ANY);
	_master_conn_addr.sin_port = htons(MASTER_CONN_PORT);

	_master_addr.sin_family = AF_INET;
	_master_addr.sin_port = htons(_master_port);
	_LOG("start registeration\n");
	if( 0  ==  inet_aton(master_ip.c_str(), &_master_addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error");
	}
	try
	{
		_master_socket=Client::_connect_to_server(_master_conn_addr, _master_addr); 
	}
	catch(std::runtime_error &e)
	{
		throw;
	}
	Send(_master_socket, REGIST);
	Send_flush(_master_socket, _memory);
	ssize_t id=-1;
	Recv(_master_socket, id);
	Server::_add_socket(_master_socket);
	_LOG("registeration finished, id=%ld\n", id);
	return id; 
}

int IOnode::_parse_new_request(int sockfd,
		const struct sockaddr_in& client_addr)
{
	int request, ans=SUCCESS; 
	Recv(sockfd, request); 
	switch(request)
	{
/*	case SERVER_SHUT_DOWN:
		ans=SERVER_SHUT_DOWN; break; */
	case READ_FILE:
		_send_data(sockfd);break;
	case NEW_CLIENT:
		_regist_new_client(sockfd);break;
	default:
		break; 
	}
	return ans; 
}

//request from master
int IOnode::_parse_registed_request(int sockfd)
{
	int request, ans=SUCCESS; 
	Recv(sockfd, request); 
	switch(request)
	{
	case READ_FILE:
		_send_data(sockfd);break;
	case WRITE_FILE:
		_receive_data(sockfd);break;
	case OPEN_FILE:
		_open_file(sockfd);break;
	case APPEND_BLOCK:
		_append_new_block(sockfd);break;
	case RENAME:
		_rename(sockfd);break;
	case FLUSH_FILE:
		_flush_file(sockfd);break;
	case CLOSE_FILE:
		_close_file(sockfd);break;
	case CLOSE_CLIENT:
		_close_client(sockfd);break;
	case TRUNCATE:
		_truncate_file(sockfd);break;
	case UNLINK:
		_unlink(sockfd);break;
	default:
		break; 
	}
	return ans; 
}

int IOnode::_send_data(int sockfd)
{
	ssize_t file_no;
	off64_t start_point;
	off64_t offset;
	size_t size;
	Recv(sockfd, file_no);
	Recv(sockfd, start_point);
	Recv(sockfd, offset);
	Recv(sockfd, size);
	_LOG("request for send data\n");
	_DEBUG("file_no=%ld, start_point=%ld, offset=%ld, size=%lu\n", file_no, start_point, offset, size);

	_DEBUG("file path=%s\n", _file_path[file_no].c_str());
	try
	{
		const std::string& path = _file_path.at(file_no);
		block* requested_block=_files.at(file_no).at(start_point);
		if(INVALID == requested_block->valid)
		{
			requested_block->allocate_memory();
			_read_from_storage(path, requested_block);
			requested_block->valid = VALID;
		}
		Send_flush(sockfd, SUCCESS);
		//fwrite((requested_block->data)+offset, sizeof(char), size, stderr);
		Sendv_pre_alloc_flush(sockfd, reinterpret_cast<char*>(requested_block->data)+offset, size);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		_DEBUG("out of range\n");
		Send_flush(sockfd, FILE_NOT_FOUND);
		return FAILURE;
	}
}

int IOnode::_receive_data(int clientfd)
{
	ssize_t file_no;
	off64_t start_point;
	off64_t offset;
	size_t size;
	Recv(clientfd, file_no);
	Recv(clientfd, start_point);
	Recv(clientfd, offset);
	Recv(clientfd, size);
	_LOG("request for receive data\n");
	_DEBUG("file_no=%ld, start_point=%ld, offset=%ld, size=%lu\n", file_no, start_point, offset, size);

	try
	{
		block_info_t &blocks=_files.at(file_no);
		block* _block=blocks.at(start_point);
		if(INVALID == _block->valid)
		{
			_block->allocate_memory();
			_block->data_size=_read_from_storage(_file_path.at(file_no), _block);
			_block->valid = VALID;
		}
		Send_flush(clientfd, SUCCESS);
		Recvv_pre_alloc(clientfd, reinterpret_cast<char*>(_block->data)+offset, size);
		if(_block->data_size < offset+size)
		{
			_block->data_size = offset+size;
		}
		_block->dirty_flag=DIRTY;
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		Send_flush(clientfd, FILE_NOT_FOUND);
		return FAILURE;
	}
}

int IOnode::_open_file(int sockfd)
{
	ssize_t file_no; 
	int flag=0;
	char *path_buffer=NULL; 
	int count=0;
	Recv(sockfd, file_no);
	Recv(sockfd, flag);
	Recvv(sockfd, &path_buffer); 
	_DEBUG("openfile fileno=%ld, path=%s\n", file_no, path_buffer);
	block_info_t& blocks=_files[file_no]; 
	if(_file_path.end() == _file_path.find(file_no))
	{
		_file_path.insert(std::make_pair(file_no, std::string(path_buffer)));
	}
	Recv(sockfd, count);
	for(int i=0;i<count;++i)
	{
		_append_block(sockfd, blocks);
	}
	delete[] path_buffer; 
	Recv(sockfd, count);
	Send_flush(sockfd, SUCCESS);
	return SUCCESS; 
}

int IOnode::_close_file(int sockfd)
{
	ssize_t file_no;
	Recv(sockfd, file_no);
	Send_flush(sockfd, SUCCESS);
	try
	{
		std::string &path=_file_path.at(file_no);
		block_info_t &blocks=_files.at(file_no);
		_DEBUG("close file, path=%s\n", path.c_str());
		for(block_info_t::iterator it=blocks.begin();
				it != blocks.end();++it)
		{
			block* _block=it->second;
			if(DIRTY == _block->dirty_flag)
			{
				_write_to_storage(path, _block);
				puts((char*)_block->data);
			}
			delete _block;
		}
		_files.erase(file_no);
		_file_path.erase(file_no);
		//Send(sockfd, SUCCESS);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		return FAILURE;
	}
}

int IOnode::_rename(int sockfd)
{
	ssize_t file_no; 
	char *new_path=NULL; 
	Recv(sockfd, file_no);
	Recvv(sockfd, &new_path);
	_DEBUG("rename file_no =%ld, new_path=%s\n", file_no, new_path);
	_file_path.erase(file_no);
	_file_path.insert(std::make_pair(file_no, new_path));
	delete[] new_path; 
	Send_flush(sockfd, SUCCESS);
	return SUCCESS; 
}

void IOnode::_append_block(int sockfd, block_info_t& blocks)
{
	off64_t start_point;
	size_t data_size;
	Recv(sockfd, start_point); 
	Recv(sockfd, data_size); 
	_DEBUG("append request from Master\n");
	_DEBUG("start_point=%lu, data_size=%lu\n", start_point, data_size);
	blocks.insert(std::make_pair(start_point, new block(start_point, data_size, CLEAN, INVALID)));
	return ;
}

int IOnode::_append_new_block(int sockfd)
{
	int count=0;
	ssize_t file_no;
	off64_t start_point;
	size_t data_size;
	Recv(sockfd, file_no);
	Send_flush(sockfd, static_cast<int>(SUCCESS));
	Recv(sockfd, count);
	try
	{
		block_info_t &blocks=_files.at(file_no);
		for(int i=0;i<count;++i)
		{
			Recv(sockfd, start_point);
			Recv(sockfd, data_size);
			blocks.insert(std::make_pair(start_point, new block(start_point, data_size, CLEAN, INVALID)));
		}
		Recv(sockfd, count);
		Send_flush(sockfd, SUCCESS);
	}
	catch(std::out_of_range &e)
	{
		return FAILURE;
	}
	return SUCCESS;
}

size_t IOnode::_read_from_storage(const std::string& path, block* block_data)throw(std::runtime_error)
{
	off64_t start_point=block_data->start_point;
	std::string real_path=_get_real_path(path);
	int fd=open64(real_path.c_str(), O_RDONLY); 
	_DEBUG("%s\n", real_path.c_str());
	if(-1 == fd)
	{
		perror("File Open Error"); 
		return 0;
	}
	if(-1  == lseek(fd, start_point, SEEK_SET))
	{
		perror("Seek"); 
		return 0;
	}
	ssize_t ret; 
	struct iovec vec;
	char *buffer=reinterpret_cast<char*>(block_data->data);
	size_t size=block_data->data_size;
	vec.iov_base=block_data->data;
	vec.iov_len=size;
	while(0 != size && 0!=(ret=readv(fd, &vec, 1)))
	{
		if(ret  == -1)
		{
			if(EINTR == errno)
			{
				continue; 
			}
			perror("read"); 
			break; 
		}
		vec.iov_base=buffer+ret;
		size-=ret;
		vec.iov_len=size;
	}
	close(fd);
	return block_data->data_size-size;
}

IOnode::block* IOnode::_buffer_block(off64_t start_point, size_t size)throw(std::runtime_error)
{
	try
	{
		block *new_block=new block(start_point, size, DIRTY, VALID); 
		return new_block; 
	}
	catch(std::bad_alloc &e)
	{
		throw std::runtime_error("Memory Alloc Error"); 
	}
}

size_t IOnode::_write_to_storage(const std::string& path, const block* block_data)throw(std::runtime_error)
{
	std::string real_path=_get_real_path(path);
	int fd = open64(real_path.c_str(),O_WRONLY);
	if( -1 == fd)
	{
		perror("Open File");
		throw std::runtime_error("Open File Error\n");
	}
	off64_t pos;
	if(-1 == (pos=lseek64(fd, block_data->start_point, SEEK_SET)))
	{
		perror("Seek"); 
		throw std::runtime_error("Seek File Error"); 
	}
	struct iovec iov;
	iov.iov_base=const_cast<void*>(block_data->data);
	iov.iov_len=block_data->data_size;
	writev(fd, &iov, 1);
	close(fd);
	return block_data->data_size;
}

int IOnode::_flush_file(int sockfd)
{
	ssize_t file_no;
	Recv(sockfd, file_no);
	std::string &path=_file_path.at(file_no);
	_DEBUG("flush file file_no=%ld, path=%s\n", file_no, path.c_str());
	Send_flush(sockfd, SUCCESS);
	try
	{
		block_info_t &blocks=_files.at(file_no);
		for(block_info_t::iterator it=blocks.begin();
				it != blocks.end();++it)
		{
			block* _block=it->second;
			if(DIRTY == _block->dirty_flag)
			{
				_write_to_storage(path, _block);
				_block->dirty_flag=CLEAN;
			}
		}
		return SUCCESS;
	}
	catch(std::out_of_range& e)
	{
		return FAILURE;
	}
	//Send(sockfd, SUCCESS);
}

int IOnode::_regist_new_client(int sockfd)
{
	_LOG("new client\n");
	Server::_add_socket(sockfd);
	Send_flush(sockfd, SUCCESS);
	return SUCCESS;
}

int IOnode::_close_client(int sockfd)
{
	_LOG("close client\n");
	Server::_delete_socket(sockfd);
	Send_flush(sockfd, SUCCESS);
	close(sockfd);
	return SUCCESS;
}

int IOnode::_unlink(int sockfd)
{
	ssize_t file_no;
	_LOG("unlink\n");
	Recv(sockfd, file_no);
	_LOG("file no=%ld\n", file_no);
	int ret=_remove_file(file_no);	
	Send_flush(sockfd, SUCCESS);
	return ret;

}

inline std::string IOnode::_get_real_path(const char* path)const
{
	return _mount_point+std::string(path);
}

inline std::string IOnode::_get_real_path(const std::string& path)const
{
	return _mount_point+path;
}

int IOnode::_truncate_file(int sockfd)
{
	_LOG("truncate file\n");
	off64_t start_point;
	ssize_t fd;
	Recv(sockfd, fd);
	Recv(sockfd, start_point);
	_LOG("fd=%ld, start_point=%ld\n", fd, start_point);
	block_info_t& file=_files[fd];
	delete file[start_point];
	return SUCCESS;
}

int IOnode::_remove_file(ssize_t file_no)
{
	file_blocks_t::iterator it=_files.find(file_no);
	_file_path.erase(file_no);
	for(block_info_t::iterator block_it=it->second.begin();
			block_it!=it->second.end();++block_it)
	{
		delete block_it->second;
	}
	_files.erase(it);
	return SUCCESS;
}
