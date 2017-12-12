/*
 * IOnode.h
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 *      this file define I/O node class in I/O burst buffer
 */

/*
 * Copyright (c) 2017, Tokyo Institute of Technology
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
 * All rights reserved. 
 * 
 * This file is part of HuronFS.
 * 
 * Please also read the file "LICENSE" included in this package for 
 * Our Notice and GNU Lesser General Public License.
 * 
 * This program is free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License (as published by the Free Software 
 * Foundation) version 2.1 dated February 1999. 
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */


#ifndef IONODE_H_
#define IONODE_H_

#include <string>
#include <sys/types.h>
#include <stdexcept>
#include <exception>
#include <stdlib.h>
#include <map>
#include <vector>

#include "Server.h"
#include "CBB_data_sync.h"
#include "Comm_api.h"
#include "CBB_error.h"
#include "IOnode_basic.h"
#include "CBB_swap.h"

namespace CBB
{
	namespace IOnode
	{
		class IOnode:
			public 	Common::Server,
			public	Common::CBB_data_sync,
			public  Common::CBB_swap<block>
		{
			//API
			public:
				IOnode(const std::string& 	master_ip,
				       int 			master_port)
				       throw(std::runtime_error);
				virtual ~IOnode();
				int start_server();

				typedef std::map<ssize_t, Common::comm_handle> node_handle_pool_t; //map node id: handle
				typedef std::map<ssize_t, std::string> node_ip_t; //map node id: ip

				//nested class
			private:

				//map: file_no: struct file
				typedef std::map<ssize_t, file> file_t; 

				typedef std::vector<file_t::iterator> remove_file_list_t;

				static const char * IONODE_MOUNT_POINT;
				static const char * IONODE_MEMORY_LIMIT;

				//private function
			private:
				//don't allow copy
				IOnode(const IOnode&); 
				//unregister IOnode from master
				int _unregister();
				//register IOnode to master,  on success return IOnode_id,  on failure throw runtime_error
				ssize_t _register(Common::communication_queue_array_t* output_queue,
						Common::communication_queue_array_t* input_queue) throw(std::runtime_error);
				//virtual int _parse_new_request(int sockfd,
				//		const struct sockaddr_in& client_addr); 
				virtual int _parse_request(Common::extended_IO_task* new_task)override final;

				virtual int remote_task_handler(Common::remote_task* new_task)override final;
				virtual int data_sync_parser(Common::data_sync_task* new_task)override final;
				virtual void configure_dump()override final;
				virtual CBB_error connection_failure_handler(
						Common::extended_IO_task* new_task)override final;
				virtual size_t free_data(block* data)override final;
				virtual bool need_writeback(block* data)override final;
				virtual size_t writeback(block* data, const char* mode)override final;
				virtual int interval_process()override final;
				virtual int after_large_transfer(void* context)override final;

				int _send_data(Common::extended_IO_task* new_task);
				int _receive_data(Common::extended_IO_task* new_task);
				int _open_file(Common::extended_IO_task* new_task);
				int _close_file(Common::extended_IO_task* new_task);
				int _flush_file(Common::extended_IO_task* new_task);
				int _rename(Common::extended_IO_task* new_task);
				int _truncate_file(Common::extended_IO_task* new_task);
				int _append_new_block(Common::extended_IO_task* new_task);
				int _register_new_client(Common::extended_IO_task* new_task);
				int _close_client(Common::extended_IO_task* new_task);
				int _unlink(Common::extended_IO_task* new_task);
				int _node_failure(Common::extended_IO_task* new_task);
				int _heart_beat(Common::extended_IO_task* new_task);
				int _get_sync_data(Common::extended_IO_task* new_task);
				int _register_new_IOnode(Common::extended_IO_task* new_task);
				int _promoted_to_primary(Common::extended_IO_task* new_task);
				int _replace_replica(Common::extended_IO_task* new_task);
				int _remove_IOnode(Common::extended_IO_task* new_task);

				int _get_replica_node_info(Common::extended_IO_task* new_task, file& _file);
				int _get_IOnode_info(Common::extended_IO_task* new_task, file& _file);

				block *_buffer_block(off64_t start_point,
						size_t size)throw(std::runtime_error);

				size_t _write_to_storage(block* block_data, const char* mode)throw(std::runtime_error); 
				size_t _read_from_storage(const  std::string& path,
							  file&	 file_stat,
							  block* block_data)throw(std::runtime_error);
				void _append_block(Common::extended_IO_task* new_task,
						   file& 		     file_stat)throw(std::runtime_error);
				virtual std::string _get_real_path(const char* path)const override final;
				std::string _get_real_path(const std::string& path)const;
				int _remove_file(ssize_t file_no);

				Common::comm_handle_t 
					_connect_to_new_IOnode(ssize_t destination_node_id,
							       ssize_t my_node_id,
							       const char* node_ip);

				//send data to new replica node after IOnode fail over
				int _sync_init_data(Common::data_sync_task* new_task);
				int _sync_write_data(Common::data_sync_task* new_task);

				int _sync_data(file& file,
						off64_t start_point,
						off64_t offset,
						ssize_t size,
						int	receiver_id,
						Common::comm_handle_t handle,
						Common::send_buffer_t* send_buffer);
				int _send_sync_data(Common::comm_handle_t handle,
						file* 	requested_file,
						off64_t start_point,
						off64_t offset,
						ssize_t size);
				size_t update_block_data(block_info_t& 		   blocks,
							 file& 			   file,
							 off64_t 		   start_point,
							 off64_t 		   offset,
							 size_t 		   size,
							 Common::extended_IO_task* new_task);

				int _setup_queues();
				int _get_sync_response();
				int update_access_order(block* requested_file, bool dirty);
				int add_write_back();
				bool have_dirty_page()const;
				void new_dirty_page();
				void clear_dirty_page();
				bool has_memory_left(size_t size)const;
				size_t allocate_memory_for_block(block* requested_block);
				size_t free_memory(size_t size);
				block* create_new_block(off64_t	 start_point,
						size_t	 data_size,
						bool 	 dirty_flag,
						bool 	 valid,
						file* 	 file_stat);
				size_t _set_memory_limit(const char* limit_string)throw(std::runtime_error);
				int remove_files();//removes delated files during the interval
				int _open_remote_file(file* 			file_stat,
						      const std::string&	real_path,
						      int			mode);
				int _close_remote_file(file* file_stat,
						       int   mode);
				bool open_too_many_files()const;

				//private member
			private:

				ssize_t 	   	my_node_id; //node id
				file_t 		   	_files;
				int 		   	_current_block_number;
				int 		   	_MAX_BLOCK_NUMBER;
				//size_t 		   	_memory; //remain available memory; 

				std::string		master_uri;

				Common::comm_handle	master_handle;
				Common::comm_handle	my_handle;

				std::string 	   	_mount_point;
				node_handle_pool_t 	IOnode_handle_pool;
				std::vector<block*> 	writeback_queue;
				int			writeback_status;
				bool			dirty_pages;
				Common::memory_pool	memory_pool_for_blocks;
				remove_file_list_t	remove_file_list;
				int			open_file_count;
		};


		inline bool IOnode::
			need_writeback(block* data)
		{
			data->wrlock();
			bool ret = (DIRTY == data->dirty_flag);
			_DEBUG("need to write back %p, %s\n", data, ret?"true":"false");
			data->unlock();
			return ret;
		}

		inline bool IOnode::
			have_dirty_page()const
		{
			return dirty_pages;
		}

		inline void IOnode::
			new_dirty_page()
		{
			dirty_pages=true;
		}

		inline void IOnode::
			clear_dirty_page()
		{
			dirty_pages=false;
		}

		inline bool IOnode::
			has_memory_left(size_t size)const
		{
			return this->memory_pool_for_blocks.has_memory_left(size);
		}

		inline int IOnode::
			after_large_transfer(void* ptr)
		{
			block* block_ptr=reinterpret_cast<block*>(ptr);
			block_ptr->finish_receiving();
			_DEBUG("finish receiving %p\n", block_ptr);
			return SUCCESS;
		}

		inline bool IOnode::
			open_too_many_files()const
		{
			return MAX_OPEN_FILE_COUNT <= this->open_file_count;
		}
	}
}

#endif
