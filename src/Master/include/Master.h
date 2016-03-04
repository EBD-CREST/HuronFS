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
//#include "CBB_map.h"
//#include "CBB_set.h"
#include "Master_basic.h"
#include "CBB_heart_beat.h"
#include "CBB_serialization.h"

namespace CBB
{
	namespace Master
	{

		class Master:
			Common::Server,
			Common::CBB_heart_beat,
			Common::CBB_serialization
		{
			//API
			public:
				Master()throw(std::runtime_error);
				virtual ~Master();
				int start_server();
				void stop_server();
			private:
				//map file_no, vector<start_point>
				//unthread-safe
				//map file_no:file_info need delete
				typedef std::map<ssize_t, open_file_info*> File_t; 
				//map file_path: file_stat
				//typedef CBB_map<std::string, file_stat> file_stat_t; 
				//map socket, node_info	need delete
				typedef std::map<int, node_info*> IOnode_sock_t; 

				//map node_id:node_info need delete
				typedef std::map<ssize_t, node_info*> IOnode_t; 
				//map file_no: ip
				//typedef std::set<ssize_t> node_id_pool_t;
				//map node_id, block_info
				//typedef std::map<ssize_t, block_info_t> node_block_map_t;

				//unthread-safe
				typedef std::set<std::string> dir_t;

				static const char* MASTER_MOUNT_POINT;
				static const char* MASTER_NUMBER;
				static const char* MASTER_TOTAL_NUMBER;
				static const char* MASTER_BACKUP_POINT;

			private:

				//virtual int _parse_new_request(int socketfd,
				//			const struct sockaddr_in& client_addr);
				virtual int _parse_request(Common::extended_IO_task* new_task) override final;

				//request parser
				int _parse_regist_IOnode(Common::extended_IO_task* new_task);
				int _parse_new_client(Common::extended_IO_task* new_task);
				int _parse_open_file(Common::extended_IO_task* new_task);
				int _parse_read_file(Common::extended_IO_task* new_task);
				int _parse_write_file(Common::extended_IO_task* new_task);
				int _parse_flush_file(Common::extended_IO_task* new_task);
				int _parse_close_file(Common::extended_IO_task* new_task);
				//int _parse_node_info(int clientfd)const;
				int _parse_attr(Common::extended_IO_task* new_task);
				int _parse_readdir(Common::extended_IO_task* new_task);
				int _parse_unlink(Common::extended_IO_task* new_task);
				int _parse_rmdir(Common::extended_IO_task* new_task);
				int _parse_access(Common::extended_IO_task* new_task);
				int _parse_mkdir(Common::extended_IO_task* new_task);
				int _parse_rename(Common::extended_IO_task* new_task);
				int _parse_close_client(Common::extended_IO_task* new_task);
				int _parse_truncate_file(Common::extended_IO_task* new_task);
				int _parse_rename_migrating(Common::extended_IO_task* new_task);
				int _parse_node_failure(Common::extended_IO_task* new_task);
				int _parse_IOnode_failure(Common::extended_IO_task* new_task);

				//remote request parsar
				int _remote_rename(Common::remote_task* new_task);
				int _remote_rmdir(Common::remote_task* new_task);
				int _remote_unlink(Common::remote_task* new_task);
				int _remote_mkdir(Common::remote_task* new_task);

				int _unregist_IOnode(node_info* IOnode_info);
				int _remove_IOnode_buffered_file(node_info* IOnode_info);
				int _remove_IOnode(node_info* IOnode_info);
				int _close_client(int socket);
				int _buffer_all_meta_data_from_remote(const char* mount_point)throw(std::runtime_error);
				int _dfs_items_in_remote(DIR* current_remote_directory,
						char* file_path,
						const char* file_relative_path,
						size_t offset)throw(std::runtime_error);

				void _send_block_info(Common::extended_IO_task* new_task,
						const node_info_pool_t& node_info_pool,
						const block_list_t& block_set)const;
				/*void _send_append_request(ssize_t file_no,
						const node_block_map_t& append_node_block);*/
				int _send_open_request_to_IOnodes(struct open_file_info& file,
						node_info* current_node,
						const block_list_t& block_info,
						const node_info_pool_t& node_info_set);
				int _send_replica_nodes_info(Common::extended_IO_task* output, const node_info_pool_t& IOnodes_set);


				ssize_t _add_IOnode(const std::string& node_ip,
						std::size_t avaliable_memory,
						int socket);
				ssize_t _delete_IOnode(int socket);
				const node_info_pool_t& _open_file(const char* file_path,
						int flag,
						ssize_t& file_no,
						int exist_flag)throw(std::runtime_error, std::invalid_argument, std::bad_alloc);
				ssize_t _get_node_id(); 
				ssize_t _get_file_no(); 
				void _release_file_no(ssize_t fileno);
				int _allocate_new_blocks_for_writing(open_file_info& file,
						off64_t start_point,
						size_t size);

				IOnode_t::iterator _find_by_ip(const std::string& ip);
				void _create_file(const char* file_path,
						mode_t mode)throw(std::runtime_error); 
				open_file_info* _create_new_open_file_info(ssize_t file_no,
						int flag,
						Master_file_stat* file_status)throw(std::invalid_argument);
				Master_file_stat* _create_new_file_stat(const char* relative_path,
						int exist_flag)throw(std::invalid_argument);

				node_info* _select_IOnode(ssize_t file_no,
						int num_of_IOnodes,
						node_info_pool_t& node_id_pool); 

				int _create_block_list(size_t file_size,
						size_t block_size,
						block_list_t& block_list,
						node_info_pool_t& node_list);

				int _select_node_block_set(open_file_info& file,
						off64_t start_point,
						size_t size,
						node_info_pool_t& node_id_pool,
						block_list_t& node_set)const;
				int _allocate_one_block(const struct open_file_info &file)throw(std::bad_alloc);
				void _append_block(struct open_file_info& file,
						off64_t start_point,
						size_t size);

				int _remove_open_file(ssize_t fd);
				int _remove_open_file(open_file_info* file_info);
				int _remove_file(const std::string& file_path);
				int _rename_local_file(Master_file_stat& file_stat, const std::string& new_relative_path);
				void _send_rename_request(node_info* node, ssize_t file_no, const std::string& new_relative_path);
				void _send_remove_request(node_info* IOnode, ssize_t file_no);
				void _send_truncate_request(node_info* node, ssize_t fd, off64_t block_start_point, ssize_t size);

				int _recreate_replicas(node_info* node_info);
				node_info* _allocate_replace_IOnode(node_info_pool_t& node_info_pool);
				int _resend_replica_nodes_info_to_new_node(open_file_info* file_info, node_info* primary_replica_node, node_info* new_IOnode);
				int _replace_replica_nodes_info(open_file_info* file_info, node_info* new_IOnode, node_info* replaced_info);
				int _send_remove_IOnode_request(node_info* removed_IOnode);

				virtual std::string _get_real_path(const char* path)const override final;
				virtual std::string _get_real_path(const std::string& path)const override final;
				virtual int remote_task_handler(Common::remote_task* new_task)override final;
				virtual int get_IOnode_socket_map(socket_map_t& map)override final;
				virtual int node_failure_handler(int socket)override final;

				ssize_t _select_IOnode_for_IO(open_file_info& file);
				dir_t _get_file_stat_from_dir(const std::string& path);
				int _setup_queues();
				node_info* _get_next_IOnode();	
				int _get_my_thread_id()const;

				int _IOnode_failure_handler(node_info* IOnode_info);
				
				void flush_file_stat();

				std::string _get_backup_path(const std::string& path)const;
				int _create_new_backup_file(const std::string& path, mode_t mode);
				int _update_backup_file_size(const std::string& path, size_t size);
			private:
				//IOnode info map node_id:node info
				IOnode_t _registed_IOnodes;
				//all file status map, file_path: file status
				file_stat_pool_t _file_stat_pool;

				//buffered files info map, file_path: opened file status
				File_t _buffered_files; 
				//socket IOnode info map, socket, IOnode info
				IOnode_sock_t _IOnode_socket; 

				ssize_t _file_number; 
				bool* _node_id_pool; 
				bool* _file_no_pool; 
				ssize_t _current_node_number; 
				ssize_t _current_file_no; 
				std::string _mount_point;
				std::string metadata_backup_point;
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

		inline std::string Master::_get_backup_path(const std::string& path)const
		{
			return metadata_backup_point+path;
		}

		inline int Master::_get_my_thread_id()const
		{
			return 0;
		}
	}
}

#endif
