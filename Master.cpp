/*OB
 * Master.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */

#include "Master.h"
#include <unistd.h>

Master::node_info::node_info(std::string ip, std::size_t total_memory,  unsigned int node_id):
	ip(ip), 
	blocks(block_info()), 
	avaliable_memory(total_memory), 
	total_memory(total_memory), 
	node_id(node_id)
{}

Master::Master():
	IOnodes(std::vector<node_info>()), 
	number_nodes(0), 
	files(file_info()), 
	_id_pool(new bool[MAX_NODE_NUMBER]), 
	_now_node_number(0)
{
	memset(_id_pool, 0, MAX_NODE_NUMBER*sizeof(bool)); 
}

int Master::_add_IO_node(std::string node_ip, std::size_t total_memory)throw(std::bad_alloc)
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
	IOnodes.push_back(node_info(node_ip, total_memory, node_id));
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
	
