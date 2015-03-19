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

#include "Server.h"
#include "Client.h"


class IOnode:public Server, Client
{
//API
public:
	IOnode(const std::string& master_ip,  int master_port) throw(std::runtime_error);
	//~IOnode();
//nested class
private:

	struct block
	{
		block(off64_t start_point, size_t size, bool dirty_flag, bool valid) throw(std::bad_alloc);
		~block();
		block(const block&);

		void allocate_memory()throw(std::bad_alloc);
		size_t data_size;
		void* data;
		off64_t  start_point;
		bool dirty_flag;
		bool valid;
	};
	//map: start_point : block*
	typedef std::map<off64_t, block*> block_info_t; 
	//map: file_no: block_info
	typedef std::map<ssize_t, block_info_t > file_blocks_t; 
	
	typedef std::map<ssize_t, std::string> file_path_t;

	static const char * IONODE_MOUNT_POINT;

//private function
private:
	//don't allow copy
	IOnode(const IOnode&); 
	int _connect_to_master()throw(std::runtime_error);
	//regist IOnode to master,  on success return IOnode_id,  on failure throw runtime_error
	ssize_t _regist(const std::string&  master, int master_port) throw(std::runtime_error);
	//unregist IOnode from master
	virtual int _parse_new_request(int sockfd, const struct sockaddr_in& client_addr); 
	virtual int _parse_registed_request(int sockfd); 
	virtual std::string _get_real_path(const char* path)const;
	std::string _get_real_path(const std::string& path)const;

	int _send_data(int sockfd);
	int _open_file(int sockfd); 
	//int _read_file(int sockfd);
	int _IOrequest_from_master(int sockfd);
	int _flush_file(int sockfd);
	int _close_file(int sockfd);
	int _append_new_block(int sockfd);
	int _regist_new_client(int sockfd);
	int _close_client(int sockfd);
	int _truncate_file(int soctfd);
	block *_buffer_block(off64_t start_point, size_t size)throw(std::runtime_error);
	int _receive_data(int sockfd); 
	//int _write_back_file(int sockfd);

	size_t _write_to_storage(const std::string& path, const block* block_data)throw(std::runtime_error); 
	size_t _read_from_storage(const std::string& path, block* block_data)throw(std::runtime_error);
	void _append_block(int sockfd, block_info_t& blocks);

//private member
private:

	//node id
	ssize_t _node_id;
	/*block_info _blocks;
	file_info _files;*/

	file_blocks_t _files;
	file_path_t _file_path;	
	int _current_block_number;
	int _MAX_BLOCK_NUMBER;
	//remain available memory; 
	size_t _memory;
	//master_conn_port
	int _master_port;
	//IO-node_server_address
	struct sockaddr_in _master_conn_addr;
	struct sockaddr_in _master_addr;
	std::string _mount_point;
};

#endif
