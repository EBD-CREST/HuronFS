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
#include <set>
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
	struct file_info; 
	struct node_info; 
	//set file_no
	typedef std::set<ssize_t> file_t;
	//map file start_point:node_id
	typedef std::map<std::size_t, ssize_t> node_t; 
	//map file_path: file_no
	typedef std::map<std::string, ssize_t> file_no_t; 
	//map node_id:node_info
	typedef std::map<ssize_t, node_info> IOnode_t; 

	struct file_info
	{
		//int : node_id; 
		file_info(const std::string& path, std::size_t size); 
		std::string path; 
		node_t IOnodes;
		std::size_t size; 
	}; 

	struct node_info
	{
		node_info(const std::string& ip, std::size_t total_memory); 
		std::string ip; 
		file_t stored_files; 
		std::size_t avaliable_memory; 
		std::size_t total_memory;
	}; 

private:
	ssize_t _add_IO_node(const std::string& node_ip, std::size_t avaliable_memory);
	ssize_t _delete_IO_node(const std::string& node_ip);
	int _add_file(const std::string& file_path);
	int _get_file_blocks(const std::string& file_path);  
	ssize_t _get_node_id(); 
	void _print_node_info(int socket);
	IOnode_t::iterator _find_by_ip(const std::string& ip);
	virtual void _parse_request(int fd, const struct sockaddr_in& client_addr); 

private:
	IOnode_t _registed_IOnodes;
	file_no_t _buffered_files;  
	ssize_t _number_node;
	bool *_id_pool; 
	ssize_t _now_node_number; 
};

#endif
