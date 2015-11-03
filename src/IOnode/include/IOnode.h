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
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdexcept>
#include <exception>
#include <stdlib.h>

#include "Server.h"
#include "CBB_map.h"
#include "CBB_connector.h"

namespace CBB
{
	namespace IOnode
	{
		class IOnode:public CBB::Common::Server,
		CBB::Common::CBB_connector
		{
			//API
			public:
				IOnode(const std::string& master_ip,
						int master_port) throw(std::runtime_error);
				virtual ~IOnode();
				int start_server();

				//nested class
			private:

				struct file;
				struct block
				{
					block(off64_t start_point,
							size_t size,
							bool dirty_flag,
							bool valid,
							file* file_stat) throw(std::bad_alloc);
					~block();
					block(const block&);

					void allocate_memory()throw(std::bad_alloc);
					
					size_t data_size;
					void* data;
					off64_t  start_point;
					bool dirty_flag;
					bool valid;
					int exist_flag;
					file* file_stat;
				};
				//map: start_point : block*
				typedef CBB::Common::CBB_map<off64_t, block*> block_info_t; 

				struct file
				{
					file(const char *path, int exist_flag, ssize_t file_no);
					~file();

					std::string file_path;
					int exist_flag;
					bool dirty_flag;
					ssize_t file_no;
					block_info_t blocks;
				};

				//map: file_no: struct file
				typedef CBB::Common::CBB_map<ssize_t, file> file_t; 

				static const char * IONODE_MOUNT_POINT;

				//private function
			private:
				//don't allow copy
				IOnode(const IOnode&); 
				//unregist IOnode from master
				int _unregist();
				//regist IOnode to master,  on success return IOnode_id,  on failure throw runtime_error
				ssize_t _regist(CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* input_queue) throw(std::runtime_error);
				//virtual int _parse_new_request(int sockfd,
				//		const struct sockaddr_in& client_addr); 
				virtual int _parse_request(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue); 

				virtual int remote_task_handler(CBB::Common::remote_task* new_task);

				int _send_data(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue);
				int _receive_data(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue);
				int _open_file(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue);
				int _close_file(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue);
				int _flush_file(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue);
				int _rename(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue);
				int _truncate_file(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue);
				int _append_new_block(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue);
				int _regist_new_client(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue);
				int _close_client(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue);
				int _unlink(CBB::Common::extended_IO_task* new_task,
						CBB::Common::task_parallel_queue<CBB::Common::extended_IO_task>* output_queue);
				block *_buffer_block(off64_t start_point,
						size_t size)throw(std::runtime_error);

				size_t _write_to_storage(block* block_data)throw(std::runtime_error); 

				size_t _read_from_storage(const std::string& path,
						block* block_data)throw(std::runtime_error);
				void _append_block(CBB::Common::extended_IO_task* new_task,
						file& file_stat);
				virtual std::string _get_real_path(const char* path)const;

				std::string _get_real_path(const std::string& path)const;

				int _remove_file(ssize_t file_no);

				//private member
			private:

				//node id
				ssize_t _node_id;
				/*block_info _blocks;
				  file_info _files;*/

				file_t _files;
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
				int _master_socket;
		};
	}
}

#endif
