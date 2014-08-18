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
#include <sys/types.h>
#include <stdexcept>
#include <stdlib.h>

#include "include/IO_const.h"
#include "include/Server.h"

class Master:public Server
{
	//API
public:
	Master()throw(std::runtime_error);
	~Master();
private:
	//map file_no, vector<start_point>
	typedef std::map<int, std::vector<std::size_t> > block_info; 
	typedef std::map<std::string,  int> file_info; 
	class node_info
	{
	public:
		node_info(const std::string& ip, std::size_t total_memory); 
		std::string ip; 
		block_info blocks;
		std::size_t avaliable_memory; 
		std::size_t total_memory;
	}; 
	//map node_id, node_info
	typedef std::map<int, node_info> nodes;

private:
	int _add_IO_node(const std::string& node_ip, std::size_t avaliable_memory);
	int _delete_IO_node(const std::string& node_ip);
	int _add_file(const std::string& file_path);
	int _get_file_blocks(const std::string& file_path);  
	int _get_node_id(); 
//	void _init_server()throw(std::runtime_error); 
	//support dynamic node info query
	void _command()const; 
	void _parse_input(); 
	void _print_node_info();
	nodes::iterator _find(const std::string&);
	virtual void _parse_request(int fd, const struct sockaddr_in& client_addr); 

private:
	nodes IOnodes;
	int number_node;
	file_info files; 
	bool *_id_pool; 
	int _now_node_number; 
	//struct sockaddr_in _server_addr; 
	//int _server_socket;
};

#endif