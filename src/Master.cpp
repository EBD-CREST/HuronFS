/*OB
 * Master.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */

#include <unistd.h>
#include <stdio.h>
#include <cstring>

#include "include/Master.h"
#include "include/IO_const.h"

Master::node_info::node_info(const std::string& ip, std::size_t total_memory, int node_id):
	ip(ip), 
	blocks(block_info()), 
	node_id(node_id), 
	avaliable_memory(total_memory), 
	total_memory(total_memory)
{}

Master::Master()throw(std::runtime_error):
	IOnodes(std::vector<node_info>()), 
	number_node(0), 
	files(file_info()), 
	_id_pool(new bool[MAX_NODE_NUMBER]), 
	_now_node_number(0)
{
	memset(_id_pool, 0, MAX_NODE_NUMBER*sizeof(bool)); 
	memset(&_server_addr, 0, sizeof(_server_addr)); 
	try
	{
		_init_server(); 
	}
	catch(std::runtime_error)
	{
		throw; 
	}
}

Master::~Master()
{
	stop_server(); 
}

int Master::_add_IO_node(const std::string& node_ip, std::size_t total_memory)throw(std::bad_alloc)
{
	int id=0; 
	try
	{
		id=_get_node_id(); 
	}
	catch(std::bad_alloc &e)
	{
		throw; 
	}
	IOnodes.push_back(node_info(node_ip, total_memory, id));
	return 1; 
}

int Master::_get_node_id()throw(std::bad_alloc)
{
	for(; _now_node_number<MAX_NODE_NUMBER; ++_now_node_number)
	{
		if(!_id_pool[_now_node_number])
		{
			_id_pool[_now_node_number]=true; 
			return _now_node_number; 
		}
	}
	for(_now_node_number=0;  _now_node_number<MAX_NODE_NUMBER; ++_now_node_number)
	{
		if(!_id_pool[_now_node_number])
		{
			_id_pool[_now_node_number]=true; 
			return _now_node_number;
		}
	}
	throw std::bad_alloc(); 
}

void Master::_init_server() throw(std::runtime_error)
{
	_server_addr.sin_family=AF_INET; 
	_server_addr.sin_addr.s_addr=htons(INADDR_ANY); 
	_server_addr.sin_port=htons(MASTER_PORT); 
	
	_server_socket=socket(PF_INET, SOCK_STREAM, 0); 
	if(0 > _server_socket)
	{
		throw std::runtime_error("Create Socket Failed"); 
	}

	if(0 != bind(_server_socket, reinterpret_cast<sockaddr *>(&_server_addr), sizeof(_server_addr)))
	{
		throw std::runtime_error("Server Bind Port Failed"); 
	}
	if(0 != listen(_server_socket, LENGTH_OF_LISTEN_QUEUE))
	{
		throw std::runtime_error("Server Listen Failed!"); 
	}
	return; 
}

void Master::start_server()
{
	while(1)
	{
		struct sockaddr_in client_addr; 
		socklen_t length=sizeof(client_addr); 
		int new_client=accept(_server_socket, reinterpret_cast<sockaddr *>(&client_addr), &length); 


		if( 0 > new_client)
		{
			fprintf(stderr, "Server Accept Failed\n"); 
			close(new_client); 
			continue; 
		}
		char buffer[MAX_SERVER_BUFFER]; 
		int len=recv(new_client, buffer, MAX_SERVER_BUFFER, 0); 
		if(0 > len)
		{
			fprintf(stderr, "Server Recvieve Data Failed\n"); 
			close(new_client); 
			continue; 
		}
		printf("A New Client\n"); 
		printf("Recieve Data\n%s\n", buffer); 
		close(new_client); 
	}
	return; 
}

void Master::stop_server()
{
	close(_server_socket); 
	return; 
}

void Master::_command()
{
	printf("command:\nprint_node_info\n"); 
}

void Master::_parse_input()
{
	char buffer[MAX_COMMAND_SIZE]; 
	scanf("%s", buffer); 
	if(!strcmp(buffer, "print_node_info"))
	{
		_print_node_info(); 
		return; 
	}
	_command(); 
}

void Master::_print_node_info()
{
	int count=0; 
	for(nodes::const_iterator it=IOnodes.begin(); it!=IOnodes.end(); ++it)
	{
		printf("IOnode %d:\nip=%s\ntotal_memory=%lu\navaliable_memory=%lu\n", ++count, it->ip.c_str(), it->avaliable_memory, it->total_memory);
	}
	return; 
}


