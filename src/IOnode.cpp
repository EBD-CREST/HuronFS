/*
 * IOnode.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#include "include/IOnode.h"
#include "include/IO_const.h"
#include "include/Communication.h"

IOnode::block::block(off_t start_point, size_t size) throw(std::bad_alloc):
	size(size),
	data(NULL),
	start_point(start_point)
{
	data=malloc(size);
	if( NULL == data)
	{
	   throw std::bad_alloc();
	}
	return;
}

IOnode::block::~block()
{
	free(data);
}

IOnode::block::block(const block & src):size(src.size),data(src.data), start_point(src.start_point){};

IOnode::IOnode(const std::string& my_ip, const std::string& master_ip,  int master_port) throw(std::runtime_error):
	Server(IONODE_PORT), 
	Client(), 
	_ip(my_ip),
	_node_id(-1),
	_files(file_blocks()), 
	_current_block_number(0), 
	_MAX_BLOCK_NUMBER(MAX_BLOCK_NUMBER), 
	_memory(MEMORY), 
	_master_port(MASTER_PORT)
{
	memset(&_master_conn_addr, 0, sizeof(_master_conn_addr));
	memset(&_master_addr, 0, sizeof(_master_addr));
	if(-1  ==  (_node_id=_regist(master_ip,  master_port)))
	{
		throw std::runtime_error("Get Node Id Error"); 
	}
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
	_unregist(); 
}

int IOnode::_regist(const std::string& master_ip, int master_port) throw(std::runtime_error)
{ 
	_master_conn_addr.sin_family = AF_INET;
	_master_conn_addr.sin_addr.s_addr = htons(INADDR_ANY);
	_master_conn_addr.sin_port = htons(MASTER_CONN_PORT);

	_master_addr.sin_family = AF_INET;
	_master_addr.sin_port = htons(_master_port);
	if( 0  ==  inet_aton(master_ip.c_str(), &_master_addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error");
	}
	int master_socket;
	try
	{
		master_socket=Client::_connect_to_server(_master_conn_addr, _master_addr); 
	}
	catch(std::runtime_error &e)
	{
		throw;
	}
	Send(master_socket, REGIST);
	Send(master_socket, _memory);
	int id=-1;
	Recv(master_socket, id);
	Server::_add_socket(master_socket);
	return id; 
}

void IOnode::_unregist()throw(std::runtime_error)
{
	int master_socket;
	try
	{
		master_socket=Client::_connect_to_server(_master_conn_addr, _master_addr);
	}
	catch(std::runtime_error &e)
	{
		throw;
	}
	Send(master_socket, UNREGIST);
	Server::_delete_socket(master_socket); 
	close(master_socket);
}

int IOnode::_insert_block(block_info& blocks, off_t start_point, size_t size) throw(std::bad_alloc,  std::invalid_argument)
{
	if( MAX_BLOCK_NUMBER < _current_block_number)
	{
		throw std::bad_alloc(); 
	}
	_current_block_number++; 
	block_info::iterator it; 
	for(it = blocks.begin(); it != blocks.end()  &&  start_point < it->second->start_point; ++it); 
	if(it != blocks.end()  &&  it->second->start_point  ==  start_point)
	{
		throw std::invalid_argument("block exists"); 
	}
	try
	{
		blocks.insert(std::make_pair(start_point, new block(start_point, size))); 
	}
	catch(std::bad_alloc)
	{
		throw; 
	}
	return start_point; 
}

void IOnode::_delete_block(block_info& blocks, off_t start_point)
{
	block_info::iterator iterator=blocks.find(start_point); 
	if(blocks.end() != iterator)
	{
		_memory += iterator->second->size; 
		delete iterator->second;
		blocks.erase(iterator);
	}
}

int IOnode::_add_file(int file_no) throw(std::invalid_argument)
{
	file_blocks::iterator it; 
	if(_files.end() == (it = _files.find(file_no)))
	{
		_files.insert(std::make_pair(file_no,  block_info())); 
		return 1; 
	}
	else
	{
		throw std::invalid_argument("file_no exists"); 
	}
}

void IOnode::_parse_new_request(int sockfd, const struct sockaddr_in& client_addr)
{
}

//request from master
void IOnode::_parse_registed_request(int sockfd)
{
	int request; 
	Recv(sockfd, request); 
	switch(request)
	{
	case BUFFER_FILE:
		_buffer_new_file(sockfd); break; 
	default:
		break; 
	}
	return; 
}

IOnode::block_info* IOnode::_buffer_new_file(int sockfd)
{
	size_t length=0, start_point, block_size; 
	ssize_t file_no; 
	Recv(sockfd, file_no);
	block_info *blocks=NULL; 
	Recv(sockfd, length); 
	char *path_buffer=new char[length]; 
	Recvv(sockfd, path_buffer, length); 
	Recv(sockfd, start_point); 
	Recv(sockfd, block_size); 
	if(_files.end() != _files.find(file_no))
	{
		blocks=&(_files.at(file_no)); 
	}
	else
	{
		blocks=&(_files[file_no]); 
	}
	blocks->insert(std::make_pair(start_point, _read_file(path_buffer, start_point, block_size))); 
	return blocks; 
}

IOnode::block* IOnode::_read_file(const char* path, off_t start_point, size_t size)throw(std::runtime_error)
{
	block *new_block=NULL; 
	try
	{
		new_block=new block(start_point, size); 
	}
	catch(std::bad_alloc &e)
	{
		throw std::runtime_error("Memory Alloc Error"); 
	}
	int fd=open(path, O_RDONLY); 
	if(-1 == fd)
	{
		perror("File Open Error"); 
		throw std::runtime_error("File Can not Be open"); 
	}
	if(-1  == lseek(fd, start_point, SEEK_SET))
	{
		perror("Seek"); 
		throw std::runtime_error("Seek File Error"); 
	}
	ssize_t ret; 
	char *buffer=reinterpret_cast<char*>(new_block->data); 
	while(0 != size && 0!=(ret=read(fd, buffer, size)))
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
		size -= ret; 
		buffer += ret; 
	}
	return new_block; 
}

IOnode::block* IOnode::_write_file(const char* path, off_t start_point, size_t size)throw(std::runtime_error)
{
	block *new_block=NULL; 
	try
	{
		new_block=new block(start_point, size); 
	}
	catch(std::bad_alloc &e)
	{
		throw std::runtime_error("Memory Alloc Error"); 
	}
	
	return new_block; 
}

