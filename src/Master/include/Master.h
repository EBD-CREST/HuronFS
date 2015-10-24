/*
 * Master.h
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 *      this file define Master class in I/O burst buffer
 */
#ifndef MASTER_H_
#define MASTER_H_

#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdexcept>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>

#include "CBB_const.h"
#include "Server.h"
#include "CBB_map.h"
#include "CBB_set.h"
#include "Master_basic.h"

namespace CBB
{
	namespace Master
	{

		class Master:public CBB::Common::Server
		{
			//API
			public:
				Master()throw(std::runtime_error);
				virtual ~Master();
				int start_server();
			private:
				//map file_no, vector<start_point>
				//unthread-safe
				//map file_no:file_info need delete
				typedef CBB::Common::CBB_map<ssize_t, open_file_info*> File_t; 
				//map file_path: file_stat
				//typedef CBB_map<std::string, file_stat> file_stat_t; 
				//map socket, node_info	need delete
				typedef CBB::Common::CBB_map<int, node_info*> IOnode_sock_t; 

				//map node_id:node_info need delete
				typedef CBB::Common::CBB_map<ssize_t, node_info*> IOnode_t; 
				//map file_no: ip
				typedef CBB::Common::CBB_set<ssize_t> node_id_pool_t;

				//map start_point, data_size
				typedef CBB::Common::CBB_map<off64_t, size_t> block_info_t;
				//map node_id, block_info
				typedef CBB::Common::CBB_map<ssize_t, block_info_t> node_block_map_t;

				//unthread-safe
				typedef std::set<std::string> dir_t;

				static const char* MASTER_MOUNT_POINT;
				static const char* MASTER_NUMBER;
				static const char* MASTER_TOTAL_NUMBER;

			private:

				//virtual int _parse_new_request(int socketfd,
				//			const struct sockaddr_in& client_addr);
				virtual int _parse_request(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);

				//request from new client
				int _parse_regist_IOnode(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_new_client(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);

				//request from registed client
				int _parse_open_file(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue); 
				int _parse_read_file(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_write_file(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_flush_file(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_close_file(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				//int _parse_node_info(int clientfd)const;
				int _parse_attr(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_readdir(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_unlink(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_rmdir(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_access(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_mkdir(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_rename(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_close_client(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_truncate_file(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _parse_rename_migrating(CBB::Common::IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);

				int _buffer_all_meta_data_from_remote(const char* mount_point)throw(std::runtime_error);
				int _dfs_items_in_remote(DIR* current_remote_directory,
						char* file_path,
						const char* file_relative_path,
						size_t offset)throw(std::runtime_error);

				void _send_block_info(CBB::Common::IO_task* new_task,
						const node_id_pool_t& node_id_pool,
						const node_t& node_set)const;
				void _send_append_request(ssize_t file_no,
						const node_block_map_t& append_node_block,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);
				int _send_open_request_to_IOnodes(struct open_file_info& file, CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);

				//file operation
				//void _send_node_info(int socket)const;
				//void _send_file_info(int socket)const; 
				//int _send_file_meta(int socket)const; 

				ssize_t _add_IO_node(const std::string& node_ip,
						std::size_t avaliable_memory,
						int socket);
				ssize_t _delete_IO_node(int socket);
				const node_t& _open_file(const char* file_path,
						int flag,
						ssize_t& file_no,
						int exist_flag,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue)
					throw(std::runtime_error, std::invalid_argument, std::bad_alloc);
				ssize_t _get_node_id(); 
				ssize_t _get_file_no(); 
				void _release_file_no(ssize_t fileno);

				IOnode_t::iterator _find_by_ip(const std::string& ip);
				void _create_file(const char* file_path,
						mode_t mode)throw(std::runtime_error); 
				open_file_info* _create_new_open_file_info(ssize_t file_no,
						int flag,
						Master_file_stat* file_status,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue)throw(std::invalid_argument);
				Master_file_stat* _create_new_file_stat(const char* relative_path,
						int exist_flag)throw(std::invalid_argument);
				node_t _select_IOnode(off64_t start_point,
						size_t file_size,
						size_t block_size,
						node_block_map_t& node_block_map); 

				size_t _get_block_size(size_t length);
				off64_t _get_block_start_point(off64_t start_point, size_t& size)const;

				node_t& _get_IOnodes_for_IO(off64_t start_point,
						size_t& size,
						struct open_file_info& file,
						node_t& node_set,
						node_id_pool_t& node_id_pool,
						CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue)throw(std::bad_alloc);
				int _allocate_one_block(const struct open_file_info &file)throw(std::bad_alloc);
				void _append_block(open_file_info& file,
						int node_id,
						off64_t start_point);
				int _remove_file(ssize_t fd, CBB::Common::task_parallel_queue<CBB::Common::IO_task>* output_queue);

				virtual std::string _get_real_path(const char* path)const;
				virtual std::string _get_real_path(const std::string& path)const;

				dir_t _get_file_stat_from_dir(const std::string& path);

				//void flush_map(file_stat_t& map)const;
			private:
				IOnode_t _registed_IOnodes;
				//file_stat_t _file_stat;  
				file_stat_pool_t _file_stat_pool;

				File_t _buffered_files; 
				IOnode_sock_t _IOnode_socket; 

				ssize_t _node_number;
				ssize_t _file_number; 
				bool *_node_id_pool; 
				bool *_file_no_pool; 
				ssize_t _current_node_number; 
				ssize_t _current_file_no; 
				std::string _mount_point;
				int master_number;
				int master_total_size;

				IOnode_t::iterator _current_IOnode;
		};

		inline std::string Master::_get_real_path(const char* path)const
		{
			return _mount_point+std::string(path);
		}

		inline std::string Master::_get_real_path(const std::string& path)const
		{
			return _mount_point+path;
		}

		inline size_t Master::_get_block_size(size_t size)
		{
			return BLOCK_SIZE;
			//return (size+_node_number-1)/_node_number;
		}
		inline off64_t Master::_get_block_start_point(off64_t start_point,
				size_t& size)const
		{
			off64_t block_start_point=(start_point/BLOCK_SIZE)*BLOCK_SIZE;
			size=start_point-block_start_point+size;
			return block_start_point;
		}
	}
}

#endif
