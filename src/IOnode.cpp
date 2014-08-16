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

#include "include/IOnode.h"

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
	_ip(my_ip),
	_node_id(-1),
	_files(file_blocks()), 
	_current_block_number(0), 
	_MAX_BLOCK_NUMBER(MAX_BLOCK_NUMBER), 
	_memory(MEMORY), 
	_master_port(MASTER_PORT), 
	_node_server_socket(-1), 
	_master_socket(-1) 
{
	if(-1  ==  (_node_id=_regist(master_ip,  master_port)))
	{
		throw std::runtime_error("Get Node Id Error"); 
	}
	try
	{
		_init_server(); 
	}
	catch(std::runtime_error& e)
	{
		_unregist();
		throw;
	}
}

void IOnode::_init_server() throw(std::runtime_error)
{
	memset(&_node_server_addr, 0, sizeof(_node_server_addr));
	_node_server_addr.sin_family = AF_INET; 
	_node_server_addr.sin_addr.s_addr = htons(INADDR_ANY); 
	_node_server_addr.sin_port = htons(_master_port); 
	if( 0 > (_node_server_socket = socket(PF_INET, SOCK_STREAM, 0)))
	{
		throw std::runtime_error("Create Socket Failed");  
	}
	if(0 != bind(_node_server_socket, reinterpret_cast<struct sockaddr*>(&_node_server_addr), sizeof(_node_server_addr)))
	{
		throw std::runtime_error("Server Bind Port Failed"); 
	}
	if(0 != listen(_node_server_socket, MAX_QUEUE))
	{
		throw std::runtime_error("Server Listen PORT ERROR");  
	}
	
	std::cout << "Start IO node Server" << std::endl; 
}	

IOnode::~IOnode()
{
	if(-1 != _node_server_socket)
	{
		close(_node_server_socket); 
	}
	_unregist(); 
}

int IOnode::_regist(const std::string& master_ip, int master_port) throw(std::runtime_error)
{ 
	memset(&_master_addr, 0, sizeof(_master_addr)); 
	_master_conn_addr.sin_family = AF_INET;
	_master_conn_addr.sin_addr.s_addr = htons(MASTER_CONN_PORT); 
	
	_master_socket = socket(PF_INET,  SOCK_STREAM, 0); 
	if( 0 > _master_socket)
	{
		//perror("Create Socket Failed"); 
		throw std::runtime_error("Create Socket Failed"); 
	}

	if(bind(_master_socket,  reinterpret_cast<struct sockaddr*>(&_master_conn_addr), sizeof(_master_conn_addr)))
	{
		//perror("Client Bind Port Failed");
		throw std::runtime_error("Client Bind Port Failed");  
	}
	memset(&_master_addr, 0,  sizeof(_master_addr)); 
	_master_addr.sin_family = AF_INET; 
	if( 0  ==  inet_aton(master_ip.c_str(), &_master_addr.sin_addr))
	{
		//perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error");
	}

	_master_addr.sin_port = htons(_master_port); 
	if( connect(_master_socket,  reinterpret_cast<struct sockaddr*>(&_master_addr),  sizeof(_master_addr)))
	{
		throw std::runtime_error("Can Not Connect To master");  
	}
	send(_master_socket, &REGIST, sizeof(REGIST),  0);
	char id[sizeof(int)]; 
	recv(_master_socket, id,  sizeof(id), 0);
	return atoi(id); 
}

void IOnode::_unregist()
{
	if(-1 != _master_socket)
	{
		send(_master_socket,  &UNREGIST,  sizeof(UNREGIST),  0);
		close(_master_socket); 
		_master_socket = -1; 
	}
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

void IOnode::start_server()
{
	
}
