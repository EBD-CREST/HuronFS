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

#include "include/IOnode.h"
#include "include/IO_const.h"
#include "include/Communication.h"

IOnode::block::block(std::size_t start_point, std::size_t size) throw(std::bad_alloc):size(size),data(NULL), start_point(start_point)
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

int IOnode::_connect_to_master() throw(std::runtime_error)
{
	int master_socket = socket(PF_INET,  SOCK_STREAM, 0); 
	if( 0 > master_socket)
	{
		perror("Create Socket Failed"); 
		throw std::runtime_error("Create Socket Failed"); 
	}
	if(bind(master_socket, reinterpret_cast<struct sockaddr*>(&_master_conn_addr), sizeof(_master_conn_addr)))
	{
		perror("client bind port failed\n");
		throw std::runtime_error("client bind port failed\n");
	}
	
	int count=0;
	while( MAX_CONNECT_TIME > ++count && 0 !=  connect(master_socket,  reinterpret_cast<struct sockaddr*>(&_master_addr),  sizeof(_master_addr)));
	if(MAX_CONNECT_TIME == count)
	{
		close(master_socket);
		perror("Can not Connect to Master"); 
		throw std::runtime_error("Can not Connect to Master");
	}
	return master_socket;
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
	std::cout << "Master" << master_ip <<":" << _master_port << std::endl;
	int master_socket;
	try
	{
		master_socket=_connect_to_master();
	}
	catch(std::runtime_error &e)
	{
		throw;
	}
	Send(master_socket, REGIST);
	Send(master_socket, _memory);
	int id=-1;
	Recv(master_socket, id);
	close(master_socket);
	return id; 
}

void IOnode::_unregist()throw(std::runtime_error)
{
	int master_socket;
	try
	{
		master_socket=_connect_to_master();
	}
	catch(std::runtime_error &e)
	{
		throw;
	}
	Send(master_socket, UNREGIST);
	close(master_socket);
}

int IOnode::_insert_block(block_info& blocks, std::size_t start_point, std::size_t size) throw(std::bad_alloc,  std::invalid_argument)
{
	if( MAX_BLOCK_NUMBER < _current_block_number)
	{
		throw std::bad_alloc(); 
	}
	_current_block_number++; 
	block_info::iterator it; 
	for(it = blocks.begin(); it != blocks.end()  &&  start_point < (*it)->start_point; ++it); 
	if(it != blocks.end()  &&  (*it)->start_point  ==  start_point)
	{
		throw std::invalid_argument("block exists"); 
	}
	try
	{
		blocks.push_back(new block(start_point, size)); 
	}
	catch(std::bad_alloc)
	{
		throw; 
	}
	return start_point; 
}

void IOnode::_delete_block(block_info& blocks, std::size_t start_point)
{
	block_info::iterator iterator; 
	for(iterator = blocks.begin(); iterator != blocks.end() && start_point != (*iterator)->start_point;  ++iterator); 
	if(blocks.end() != iterator)
	{
		delete *iterator;
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

void IOnode::_parse_request(int sockfd, const struct sockaddr_in& client_addr)
{
}

