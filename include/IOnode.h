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
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdexcept>
#include <exception>
#include <stdlib.h>

#include "include/Server.h"
#include "include/Client.h"

class IOnode:public Server, Client
{
//API
public:
	IOnode(const std::string& my_ip, const std::string& master_ip,  int master_port) throw(std::runtime_error);
	~IOnode();
//nested class
private:

	struct block
	{
		block(off_t start_point, size_t size) throw(std::bad_alloc);
		~block();
		block(const block&);
		size_t size;
		void* data;
		off_t  start_point;
	};
	//map: start_point : block*
	typedef std::map<off_t, block*> block_info; 
	//map: file_no: block_info
	typedef std::map<int, block_info > file_blocks; 

//private function
private:
	//don't allow copy
	IOnode(const IOnode&); 
	int _connect_to_master()throw(std::runtime_error);
	//regist IOnode to master,  on success return IOnode_id,  on failure throw runtime_error
	int _regist(const std::string&  master, int master_port) throw(std::runtime_error);
	//unregist IOnode from master
	void _unregist() throw(std::runtime_error); 
	//insert block,  on success return start_point,  on failure throw bad_alloc
	int _insert_block(block_info& blocks, off_t start_point, size_t size) throw(std::bad_alloc,  std::invalid_argument);
	//delete block
	void _delete_block(block_info& blocks, off_t start_point);  
	
	int _add_file(int file_no) throw(std::invalid_argument);  

	int _delete_file(int file_no) throw(std::invalid_argument); 
	
	virtual void _parse_new_request(int sockfd, const struct sockaddr_in& client_addr); 
	virtual void _parse_registed_request(int sockfd); 

	block_info* _buffer_new_file(int sockfd); 
	block* _read_file(const char* path, off_t start_point, size_t size)throw(std::runtime_error);
	block* _write_file(const char* path, off_t start_point, size_t size)throw(std::runtime_error);
	void _write_back_file(const char* path, const block* block_data)throw(std::runtime_error); 
//private member
private:

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
	size_t _memory;
	//master_conn_port
	int _master_port;
	//IO-node_server_address
	struct sockaddr_in _master_conn_addr;
	struct sockaddr_in _master_addr;
};

#endif
