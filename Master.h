/*
 * Master.h
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 *      this file define Master class in I/O burst buffer
 */
#ifndef MASTER_H_
#define MASTER_H_

#include <map>
#include <vector>
#include <string>


class Master
{
	//API
public:
	Master();
	Master();
	~Master();
private:
	//map file_no, vector<start_point>
	typedef std::map<int, std::vector<std::size_t> > block_info; 
	typedef std::map<std::string,  int> file_info; 

	class node_info
	{
	public:
		node_info(std::string ip, std::size_t total_memory,  unsigned int node_id); 
		~node_info();
		
		std::string ip; 
		block_info blocks;
		unsigned int node_id; 
		std::size_t avaliable_memory; 
		std::size_t total_memory; 
	}

private:
	std::vector<node_info> IOnodes;
	unsigned int number_node;
	file_info files; 
	bool *_id_pool; 
	unsigned int _now_node_number; 
	struct sockaddr_in _server_addr; 
	int _server_socket; 

private:
	int _add_IO_node(std::string node_ip, std::size_t avaliable_memory);
	int _add_file(std::string file_path);
	int _get_file_blocks(std::string file_path);  
	int _get_node_id()throw(std::bad_alloc); 
	void _init_server()throw(std::runtime_error); 
};

#endif
