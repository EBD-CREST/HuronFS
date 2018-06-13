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

#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdexcept>
#include <vector>
#include <stdexcept>

#include "CBB_communication_thread.h"
#include "CBB_fault_tolerance.h"

namespace CBB
{
	namespace Common
	{
		class Client: 
			public CBB_communication_thread, 
			protected CBB_fault_tolerance
		{
			public:
				Client(int thread_number);
				virtual ~Client();
			protected:

				void stop_client();
				int start_client();

				virtual int 
				input_from_network(Common::comm_handle_t handle,
					communication_queue_array_t* output_queue)override final;
				virtual int 
					input_from_producer(communication_queue_t* input_queue)override final;
				virtual int output_task_enqueue(extended_IO_task* output_task)override final;
				virtual communication_queue_t* 
					get_communication_queue_from_handle(comm_handle_t handle)override final;
				virtual int node_failure_handler(extended_IO_task* task)override final;
				//unused, didn't implement, don't use
				virtual int node_failure_handler(comm_handle_t handle)override final;
				virtual int connection_failure_handler(extended_IO_task* task)override final;

				virtual int _report_IOnode_failure(comm_handle_t handle)=0;

				int reply_with_handle_error(extended_IO_task* input_task,
							    int		      error);
				communication_queue_t* get_new_communication_queue();
				int release_communication_queue(communication_queue_t* queue);
				int release_communication_queue(extended_IO_task* task);
				extended_IO_task* 
					allocate_new_query(comm_handle_t handle);
				extended_IO_task* 
					allocate_new_query(const char* uri, int port);
				extended_IO_task* 
					allocate_new_query_preallocated(comm_handle_t handle, int id);
				int send_query(extended_IO_task* query);
				extended_IO_task* get_query_response(extended_IO_task* query)throw(std::runtime_error);
				int response_dequeue(extended_IO_task* response);
				int dequeue(extended_IO_task* response);
				int print_handle_error(extended_IO_task* response);
				int connect_to_server(const char* uri, int port, extended_IO_task* new_task);
				int cleanup_after_large_transfer(extended_IO_task* handle);

				communication_queue_t* get_input_queue_from_query(extended_IO_task* query);
				communication_queue_t* get_output_queue_from_query(extended_IO_task* query);
			private:
				communication_queue_array_t _communication_input_queue;
				communication_queue_array_t _communication_output_queue;
				threads_handle_map_t 	    _threads_handle_map;

		}; 

		inline extended_IO_task* Client::allocate_new_query(comm_handle_t handle)
		{
			communication_queue_t* new_queue=get_new_communication_queue();
			_DEBUG("lock queue %p\n", new_queue);
			extended_IO_task* query_task=new_queue->allocate_tmp_node();
			query_task->set_handle(handle);
			query_task->set_receiver_id(0);
			return query_task;
		}

		inline extended_IO_task* Client::
			allocate_new_query(const char* uri,
					   int	       port)
		{
			communication_queue_t* new_queue=get_new_communication_queue();
			_DEBUG("lock queue %p\n", new_queue);
			extended_IO_task* query_task=new_queue->allocate_tmp_node();
			query_task->setup_new_connection(uri, port);
			query_task->set_receiver_id(0);
			return query_task;
		}

		inline int Client::print_handle_error(extended_IO_task* response)
		{
			_DEBUG("handle error detected, handle=%p\n", response->get_handle());
			return SUCCESS;
		}

		inline extended_IO_task* Client::allocate_new_query_preallocated(comm_handle_t handle, int id)
		{
			communication_queue_t& new_queue=_communication_output_queue.at(id);
			extended_IO_task* query_task    =new_queue.allocate_tmp_node();
			query_task->set_handle(handle);
			return query_task;
		}

		inline int Client::send_query(extended_IO_task* query)
		{
			//end_recording(this, 0, READ_FILE);
			//print_log("send query enqueue", "", 0, 0);
			_DEBUG("send query %p\n", query);

			return get_output_queue_from_query(query)->task_enqueue();
		}

		inline int Client::response_dequeue(extended_IO_task* response)
		{
			dequeue(response);
			return release_communication_queue(response);
		}

		inline int Client::dequeue(extended_IO_task* response)
		{
			get_input_queue_from_query(response)->task_dequeue();
			return SUCCESS;
		}

		inline communication_queue_t* Client::get_input_queue_from_query(extended_IO_task* query)
		{
			int id=query->get_id();
			return &_communication_input_queue[id];
		}

		inline communication_queue_t* Client::get_output_queue_from_query(extended_IO_task* query)
		{
			int id=query->get_id();
			return &_communication_output_queue[id];
		}

		inline int Client::output_task_enqueue(extended_IO_task* output_task)
		{
			communication_queue_t& output_queue=_communication_output_queue.at(output_task->get_id());
			return output_queue.task_enqueue();
		}

		inline int Client::release_communication_queue(communication_queue_t* queue)
		{
			_DEBUG("release queue %p\n", queue);
			return queue->unlock_queue();
		}

		inline int Client::release_communication_queue(extended_IO_task* task)
		{
			communication_queue_t& output_queue=_communication_output_queue.at(task->get_id());
			return release_communication_queue(&output_queue);
		}
		inline int Client::
			cleanup_after_large_transfer(extended_IO_task* query)
			{
#ifdef CCI
				deregister_mem(&(query->handle));
#endif
				return SUCCESS;
			}
	}
}

#endif
