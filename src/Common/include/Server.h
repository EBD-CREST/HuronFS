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


#ifndef SERVER_H_
#define SERVER_H_

#include <netinet/in.h>
#include <stdexcept>
#include "CBB_task_parallel.h"
#include "CBB_communication_thread.h"
#include "CBB_request_handler.h"
#include "CBB_remote_task.h"
#include "CBB_fault_tolerance.h"

namespace CBB
{
	namespace Common
	{
		class Server:public CBB_communication_thread,
		public CBB_request_handler,
		public CBB_remote_task,
		public CBB_fault_tolerance
		{
			public:
				void stop_server(); 
				int start_server();

			protected:
				Server(int thread_number, int port)throw(std::runtime_error); 
				virtual ~Server(); 
				virtual int input_from_network(comm_handle_t handle,
						communication_queue_array_t* output)override final;

				virtual int input_from_producer(communication_queue_t* output)override final;
				virtual int output_task_enqueue(extended_IO_task* output_task)override final;
				virtual communication_queue_t* get_communication_queue_from_handle(comm_handle_t handle)override final;
				virtual int node_failure_handler(extended_IO_task* task)override;
				virtual int node_failure_handler(comm_handle_t handle)override;
				virtual int connection_failure_handler(extended_IO_task* task)override;

				void _init_server()throw(std::runtime_error); 

				virtual int _parse_request(extended_IO_task* new_task)=0;
				virtual std::string _get_real_path(const char* path)const=0;
				virtual std::string _get_real_path(const std::string& path)const=0;
				virtual int remote_task_handler(remote_task* new_task)=0;
				virtual void configure_dump()=0;

				int send_input_for_handle_error(comm_handle_t handle);
				communication_queue_t* get_connection_input_queue();
				communication_queue_t* get_connection_output_queue();
				extended_IO_task* get_connection_task();
				extended_IO_task* get_connection_response();
				int connection_task_enqueue(extended_IO_task* task);
				int connection_task_dequeue(extended_IO_task* task);
				extended_IO_task* init_response_task(extended_IO_task* input_task);
				communication_queue_t* get_communication_input_queue(int index);
				communication_queue_t* get_communication_output_queue(int index);
				int _recv_real_path(extended_IO_task* new_task, std::string& real_path);
				int _recv_real_relative_path(extended_IO_task* new_task, std::string& real_path, std::string &relative_path);
				//id = thread id, temporarily id=0
				extended_IO_task* allocate_output_task(int id);
				const char* _get_my_uri()const;
				void _set_uri(const char* uri);
			private:
				void _setup_server();

			protected:
				communication_queue_array_t _communication_input_queue;
				communication_queue_array_t _communication_output_queue;
				comm_handle		    _server_handle;
				//for tcp
				int			    server_port;
				threads_handle_map_t 	    _threads_handle_map;
				std::string		    my_uri;
		}; 

		inline int Server::
			output_task_enqueue(extended_IO_task* output_task)
		{
			_DEBUG("id=%d\n", output_task->get_id());
			communication_queue_t& output_queue=_communication_output_queue.at(output_task->get_id());
			return output_queue.task_enqueue();
		}

		inline extended_IO_task* Server::
			allocate_output_task(int id)
		{
			extended_IO_task* output=_communication_output_queue.at(id).allocate_tmp_node();
			output->set_receiver_id(0);
			return output;
		}

		inline communication_queue_t* Server::
			get_communication_input_queue(int index)
		{
			return &_communication_input_queue.at(index);
		}

		inline communication_queue_t* Server::
			get_communication_output_queue(int index)
		{
			return &_communication_output_queue.at(index);
		}

		inline const char* Server::
			_get_my_uri()const
		{
			return this->my_uri.c_str();
		}

		inline void Server::
			_set_uri(const char* uri)
		{
			this->my_uri=std::string(uri);
		}

		inline communication_queue_t* Server::
			get_connection_input_queue()
		{
			return &_communication_input_queue.at(CONNECTION_QUEUE_NUM);
		}

		inline communication_queue_t* Server::
			get_connection_output_queue()
		{
			return &_communication_output_queue.at(CONNECTION_QUEUE_NUM);
		}

		inline extended_IO_task* Server::
			get_connection_task()
		{
			extended_IO_task* task=get_connection_output_queue()->allocate_tmp_node();
			//fix this
			//the receiver id is temporarily 0
			task->set_receiver_id(0);
			return task;
		}

		inline int Server::
			connection_task_enqueue(extended_IO_task* task)
		{
			return get_connection_output_queue()->task_enqueue();
		}

		inline extended_IO_task* Server::
			get_connection_response()
		{
			extended_IO_task* response=get_connection_input_queue()->get_task();
			return response;
		}

		inline int Server::
			connection_task_dequeue(extended_IO_task* task)
		{
			get_connection_input_queue()->task_dequeue();
			return SUCCESS;
		}
	}
}

#endif
