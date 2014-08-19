/*OB
 * Master.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */
#include <sstream>
#include <cstring>

#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "include/Master.h"
#include "include/IO_const.h"
#include "include/Communication.h"

Master::file_info::file_info(const std::string& path, std::size_t size):
	path(path), 
	IOnodes(node_t()), 
	size(size)
{}

Master::node_info::node_info(const std::string& ip, std::size_t total_memory):
	ip(ip), 
	stored_files(file_t()), 
	avaliable_memory(total_memory), 
	total_memory(total_memory)
{}

Master::Master()throw(std::runtime_error):
	Server(MASTER_PORT), 
	_registed_IOnodes(IOnode_t()), 
	_buffered_files(file_no_t()), 
	_number_node(0), 
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
	delete _id_pool; 
	Server::stop_server(); 
}

ssize_t Master::_add_IO_node(const std::string& node_ip, std::size_t total_memory)
{
	ssize_t id=0; 
	if(-1 == (id=_get_node_id()))
	{
		return -1;
	}
	if(_registed_IOnodes.end() != _find_by_ip(node_ip))
	{
		return -1;
	}
	_registed_IOnodes.insert(std::make_pair(id, node_info(node_ip, total_memory)));
	++_number_node; 
	return id; 
}

ssize_t Master::_delete_IO_node(const std::string& node_ip)
{
	IOnode_t::iterator it=_find_by_ip(node_ip);
	if(it != _registed_IOnodes.end())
	{
		ssize_t id=it->first;
		_registed_IOnodes.erase(it);
		_id_pool[id]=false;
		if(-1 == _now_node_number)
		{
			_now_node_number=id; 
		}
		--_number_node; 
		return id;
	}
	else
	{
		return -1;
	}
}


ssize_t Master::_get_node_id()
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

void Master::_print_node_info(int clientfd)const 
{
	int count=0;
	std::ostringstream output; 
	for(IOnode_t::const_iterator it=_registed_IOnodes.begin(); it!=_registed_IOnodes.end(); ++it)
	{
		const node_info &node=it->second;
		output << "IOnode :" << ++count  << std::endl << "ip=" << node.ip << std::endl<< "total_memory=" << node.total_memory << std::endl << "avaliable_memory=" << node.avaliable_memory << std::endl;
	}
	const std::string &s=output.str(); 
	printf("%s",  s.c_str()); 
	Send(clientfd, s.size()); 
	Sendv(clientfd, s.c_str(), s.size()); 
	return; 
}

Master::IOnode_t::iterator Master::_find_by_ip(const std::string &ip)
{
	IOnode_t::iterator it=_registed_IOnodes.begin();
	for(; it != _registed_IOnodes.end() && ! (it->second.ip == ip); ++it);
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
	case PRINT_NODE_INFO:
		fprintf(stderr, "requery for IOnode info\n"); 
		_print_node_info(clientfd); 
	default:
		printf("unrecogisted communication\n");
	}
	return;
}

