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

#include "IO_const.h"

class IOnode
{
//nested class
private:

	struct block 
	{
		block(unsigned long size, int block_id) throw(std::bad_alloc);
		~block();
		block(const block&);
		unsigned long size;
		void *data;
		const unsigned int block_id;
	};
//API
public:
	IOnode(std::string my_ip, std::string master_ip,  int master_port) throw(std::runtime_error);
	~IOnode();
	int insert_block(unsigned long size);
	int delete_block();

public:
	const std::string ip;
	unsigned int node_id;
//private member
private:
	//map : block_id , block
	std::map<int,std::vector<class block> > _blocks;
	unsigned int _current_block_number;
	unsigned int MAX_BLOCK_NUMBER;
	unsigned long _memory;
	int port; 
	struct sockaddr_in node_server_addr; 
	struct sockaddr_in master_conn_addr; 
	struct sockaddr_in master_addr;
	int node_server_socket; 
	int master_socket;
//private function
private:
	//regist IOnode to master,  on success return IOnode_id,  on failure throw runtime_error
	int regist(std::string&  master, int master_port) throw(std::runtime_error);
	//start IO server on failure throw runtime_error
	void start_server() throw(std::runtime_error); 
	//unregist IOnode from master
	void unregist(); 
};

#endif
