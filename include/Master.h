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
	//map socket, node_info	
	typedef std::map<int, node_info*> IOnode_sock_t; 
	
	//map node_id:node_info
	typedef std::map<ssize_t, node_info> IOnode_t; 

	typedef std::map<ssize_t, file_info> File_t; 

	struct file_info
	{
		//int : node_id; 
		file_info(const std::string& path, std::size_t size, const node_t& IOnodes, int flag); 
		std::string path; 
		node_t IOnodes;
		std::size_t size; 
		int flag; 
	}; 

	struct node_info
	{
		node_info(const std::string& ip, std::size_t total_memory, int socket); 
		std::string ip; 
		file_t stored_files; 
		std::size_t avaliable_memory; 
		std::size_t total_memory;
		int socket; 
	}; 

private:
	ssize_t _add_IO_node(const std::string& node_ip, std::size_t avaliable_memory, int socket);
	ssize_t _delete_IO_node(const std::string& node_ip);
	const node_t& _open_file(const std::string& file_path, int flag)throw(std::runtime_error);
	int _update_file_info(int file_no); 
	int _get_file_blocks(const std::string& file_path);
	ssize_t _get_node_id(); 
	ssize_t _get_file_no(); 
	void _print_node_info(int socket)const;
	IOnode_t::iterator _find_by_ip(const std::string& ip);
	virtual void _parse_new_request(int socketfd, const struct sockaddr_in& client_addr); 
	virtual void _parse_registed_request(int socketfd); 
	node_t _read_from_storage(const std::string& file_path, ssize_t file_no); 
	node_t _select_IOnode(size_t file_size, size_t block_size); 

private:
	IOnode_t _registed_IOnodes;
	file_no_t _file_no;  
	File_t _buffered_files; 
	IOnode_sock_t _IOnode_socket; 

	ssize_t _node_number;
	ssize_t _file_number; 
	bool *_node_id_pool; 
	bool *_file_no_pool; 
	ssize_t _now_node_number; 
	ssize_t _now_file_no; 
};

#endif
