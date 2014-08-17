/*
 * IOnode.h
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 *      this file define I/O node class in I/O burst buffer
 */


#ifndef IONODE_H_
#define IONODE_H_

#include <string>
#include <map>
#include <vector>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdexcept>
#include <exception>
#include <stdlib.h>
#include <set>

#include "include/Server.h"

class IOnode:public Server
{
//API
public:
	IOnode(const std::string& my_ip, const std::string& master_ip,  int master_port) throw(std::runtime_error);
	~IOnode();

//nested class
private:

	struct block 
	{
		block(std::size_t start_point, std::size_t size) throw(std::bad_alloc);
		~block();
		block(const block&);
		std::size_t size;
		void* data;
		const std::size_t  start_point;
	};

//private function
private:
	typedef std::vector<block*> block_info; 
	typedef std::map<int, block_info > file_blocks; 
	//don't allow copy
	IOnode(const IOnode&); 
	//regist IOnode to master,  on success return IOnode_id,  on failure throw runtime_error
	int _regist(const std::string&  master, int master_port) throw(std::runtime_error);
	//unregist IOnode from master
	void _unregist() throw(std::runtime_error); 
	//insert block,  on success return start_point,  on failure throw bad_alloc
	int _insert_block(block_info& blocks, std::size_t start_point, std::size_t size) throw(std::bad_alloc,  std::invalid_argument);
	//delete block
	void _delete_block(block_info& blocks, std::size_t start_point);  
	
	int _add_file(int file_no) throw(std::invalid_argument);  

	int _delete_file(int file_no) throw(std::invalid_argument); 
	
	virtual void _parse_request(int sockfd, const struct sockaddr_in& client_addr); 
//private member
private:
	/*map : block_id , block
	typedef std::map<int, class block > block_info; 
	//map : file_no,  block_ids
	typedef std::map<int, std::set<int> > file_info; */

	//ip address
	const std::string _ip;
	//node id
	int _node_id;
	/*block_info _blocks;
	file_info _files;*/

	file_blocks _files;
	
	int _current_block_number;
	int _MAX_BLOCK_NUMBER;
	//remain available memory; 
	std::size_t _memory;
	//master_conn_port
	int _master_port;
	//IO-node_server_address
	struct sockaddr_in _master_conn_addr; 
};

#endif
