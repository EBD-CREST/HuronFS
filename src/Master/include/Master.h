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
#include <vector>

#include "CBB_const.h"
#include "Server.h"


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
	typedef std::map<off64_t, ssize_t> node_t; 

	typedef std::set<ssize_t> node_pool_t;
	//map file_path: file_no
	typedef std::map<std::string, ssize_t> file_no_t; 
	//map socket, node_info	
	typedef std::map<int, node_info*> IOnode_sock_t; 
	
	//map node_id:node_info
	typedef std::map<ssize_t, node_info> IOnode_t; 
	//map file_no:file_info
	typedef std::map<ssize_t, file_info> File_t; 
	//map file_no: ip
	typedef std::set<ssize_t> node_id_pool_t;

	//map start_point, data_size
	typedef std::map<off64_t, size_t> block_info_t;
	//map node_id, block_info
	typedef std::map<ssize_t, block_info_t> node_block_map_t;

	typedef std::vector<std::string> dir_t;

	struct file_info
	{
		file_info(const std::string& path,
				ssize_t fileno,
				size_t size,
				size_t block_size,
				const node_t& IOnodes,
				int flag); 

		ssize_t file_no;
		std::string path; 
		node_t p_node;
		node_pool_t nodes;
		size_t size; 
		size_t block_size;
		int open_count;
		//open file
		int flag; 
	}; 

	struct node_info
	{
		node_info(ssize_t id, const std::string& ip, std::size_t total_memory, int socket); 
		ssize_t node_id;
		std::string ip; 
		file_t stored_files; 
		std::size_t avaliable_memory; 
		std::size_t total_memory;
		int socket; 
	}; 

	static const char* MASTER_MOUNT_POINT;

private:
	ssize_t _add_IO_node(const std::string& node_ip, std::size_t avaliable_memory, int socket);
	ssize_t _delete_IO_node(int socket);
	const node_t& _open_file(const char* file_path, int flag, ssize_t& file_no)throw(std::runtime_error, std::invalid_argument, std::bad_alloc);
	int _update_file_info(int file_no); 
	int _get_file_blocks(const std::string& file_path);
	int _get_buffered_file_attr(int fd, struct stat* fstat)const;
	ssize_t _get_node_id(); 
	ssize_t _get_file_no(); 
	void _release_file_no(ssize_t fileno);
	void _send_node_info(int socket, const std::string& ip)const;
	void _send_block_info(int socket, const node_id_pool_t& node_id_pool, const node_t& node_set)const;
	void _send_file_info(int socket, const std::string& ip)const; 
	void _send_file_meta(int socket, const std::string& ip)const; 
	void _send_IO_request(ssize_t file_no, const file_info& file, const node_t& node_set, size_t size, int mode)const;
	void _send_append_request(ssize_t file_no, const node_block_map_t& append_node_block)const;
	void _create_file(const char* file_path, mode_t mode)throw(std::runtime_error);

	IOnode_t::iterator _find_by_ip(const std::string& ip);

	virtual int _parse_new_request(int socketfd, const struct sockaddr_in& client_addr);
	virtual int _parse_registed_request(int socketfd); 
	virtual std::string _get_real_path(const char* path)const;
	virtual std::string _get_real_path(const std::string& path)const;

	int _parse_regist_IOnode(int clientfd, const std::string& ip);
	//file operation
	int _parse_open_file(int clientfd, const std::string& ip); 
	int _parse_read_file(int clientfd, const std::string& ip);
	int _parse_write_file(int clientfd, const std::string& ip);
	int _parse_flush_file(int clientfd, const std::string& ip);
	int _parse_close_file(int clientfd, const std::string& ip);
	int _parse_node_info(int clientfd, const std::string& ip)const;
	int _parse_attr(int clientfd, const std::string& ip)const;
	int _parse_readdir(int clientfd, const std::string &ip)const;
	int _parse_unlink(int clientfd, const std::string &ip);
	int _parse_rmdir(int clientfd, const std::string &ip);

	node_t _send_request_to_IOnodes(const char *file_path, ssize_t file_no, int flag, size_t& file_length, size_t& block_size)throw(std::invalid_argument); 

	node_t _select_IOnode(off64_t start_point, size_t file_size, size_t block_size, node_block_map_t& node_block_map)const; 

	size_t _get_block_size(size_t length);
	off64_t _get_block_start_point(off64_t start_point, size_t& size)const;

	node_t& _get_IOnodes_for_IO(off64_t start_point, size_t& size, struct file_info& file, node_t& node_set, node_id_pool_t& node_id_pool)throw(std::bad_alloc);
	int _allocate_one_block(const struct file_info &file)throw(std::bad_alloc);
	void _append_block(struct file_info& file, int node_id, off64_t start_point);
	int _remove_file(int fd);

private:
	IOnode_t _registed_IOnodes;
	file_no_t _file_no;  
	File_t _buffered_files; 
	IOnode_sock_t _IOnode_socket; 

	ssize_t _node_number;
	ssize_t _file_number; 
	bool *_node_id_pool; 
	bool *_file_no_pool; 
	ssize_t _current_node_number; 
	ssize_t _current_file_no; 
	std::string _mount_point;
};

#endif
