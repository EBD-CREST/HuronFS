/*OB
 * Master.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */

#include <unistd.h>
#include <stdio.h>
#include <cstring>
#include <arpa/inet.h>

#include "include/Master.h"
#include "include/IO_const.h"
#include "include/Communication.h"

Master::node_info::node_info(const std::string& ip, std::size_t total_memory):
	ip(ip), 
	blocks(block_info()), 
	avaliable_memory(total_memory), 
	total_memory(total_memory)
{}

Master::Master()throw(std::runtime_error):
	Server(MASTER_PORT), 
	IOnodes(nodes()), 
	number_node(0), 
	files(file_info()), 
	_id_pool(new bool[MAX_NODE_NUMBER]), 
	_now_node_number(0) 
{
	memset(_id_pool, 0, MAX_NODE_NUMBER*sizeof(bool)); 
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
	Server::stop_server(); 
}

int Master::_add_IO_node(const std::string& node_ip, std::size_t total_memory)
{
	int id=0; 
	if(-1 == (id=_get_node_id()))
	{
		return -1;
	}
	if(IOnodes.end() != _find(node_ip))
	{
		return -1;
	}
	IOnodes.insert(std::make_pair(id, node_info(node_ip, total_memory)));
	return id; 
}

int Master::_delete_IO_node(const std::string& node_ip)
{
	nodes::iterator it=_find(node_ip);
	if(it != IOnodes.end())
	{
		int id=it->first;
		IOnodes.erase(it);
		_id_pool[id]=false;
		return 1;
	}
	else
	{
		return 0;
	}
}


int Master::_get_node_id()
{
	if(-1  == _now_node_number)
	{
		return -1; 
	}

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
	_now_node_number=-1;
	return -1; 
}

void Master::_command()const
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
		const node_info &node=it->second;
		printf("IOnode %d:\nip=%s\ntotal_memory=%lu\navaliable_memory=%lu\n", ++count, node.ip.c_str(), node.avaliable_memory, node.total_memory);
	}
	return; 
}

Master::nodes::iterator Master::_find(const std::string &ip)
{
	nodes::iterator it=IOnodes.begin();
	for(; it != IOnodes.end() && ! (it->second.ip == ip); ++it);
	return it;
}

void Master::_parse_request(int clientfd, const struct sockaddr_in& client_addr)
{
	int request,tmp;
	Recv(clientfd, request);
	switch(request)
	{
	case REGIST:
		fprintf(stderr, "regist IOnode\n");
		Recv(clientfd, tmp);
		Send(clientfd, _add_IO_node(std::string(inet_ntoa(client_addr.sin_addr)), tmp));break;
	case UNREGIST:
		fprintf(stderr, "unregist IOnode\n");
		_delete_IO_node(std::string(inet_ntoa(client_addr.sin_addr)));break;
	default:
		printf("unrecogisted communication\n");
	}
	_print_node_info();
	return;
}
