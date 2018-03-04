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

#ifndef CBB_COMMUNICATION_THREAD_H_
#define CBB_COMMUNICATION_THREAD_H_

#include <pthread.h>
#include <set>
#include <vector>
#include <map>

#include "CBB_task_parallel.h"
#include "CBB_profiling.h"
#include "CBB_IO_task.h"

namespace CBB
{
	namespace Common
	{

		typedef std::vector<communication_queue_t> communication_queue_array_t;
		//each thread set the handle which it is waiting on
		typedef std::vector<comm_handle_t> threads_handle_map_t;


		int Send_attr(extended_IO_task* output_task,
			      const struct stat* file_stat);
		int Recv_attr(extended_IO_task* new_task,
				struct stat* file_stat);
		class CBB_communication_thread:
			public CBB_communication,
			public CBB_profiling
		{
			public:
				typedef std::map<int, communication_queue_t*> fd_queue_map_t;
			protected:
				CBB_communication_thread()throw(std::runtime_error);
				virtual ~CBB_communication_thread();
				int start_communication_server();
				int stop_communication_server();
				int _add_event_socket(int socket);
				int _add_handle(comm_handle_t handle);
				int _add_handle(comm_handle_t handle, int op);
				int _delete_handle(comm_handle_t handle); 

				virtual int input_from_network(comm_handle_t handle,
						communication_queue_array_t* output_queue)=0;
				virtual int input_from_producer(communication_queue_t* input_queue)=0;
				virtual int output_task_enqueue(extended_IO_task* output_task)=0;
				virtual communication_queue_t* get_communication_queue_from_handle(comm_handle_t handle)=0;
				virtual int after_large_transfer(void* context, int mode);

				size_t send(extended_IO_task* new_task)throw(std::runtime_error);
				size_t send_rma(extended_IO_task* new_task) throw(std::runtime_error);
				size_t receive_message(comm_handle_t handle,
						extended_IO_task* new_task)throw(std::runtime_error);
				static void* sender_thread_function(void*);
				static void* receiver_thread_function(void*);
				//thread wait on queue event;
				//static void* queue_wait_function(void*);
				void setup(communication_queue_array_t* input_queues,
						communication_queue_array_t* output_queues);
			private:

				void set_queues(communication_queue_array_t* input_queues,
						communication_queue_array_t* output_queues);
			private:
				//map: handle
				typedef std::set<comm_handle_t> handle_pool_t; 
				int set_event_fd()throw(std::runtime_error);

			private:
				int keepAlive;
				int sender_epollfd;
				int receiver_epollfd; 
				int server_socket;
				handle_pool_t _handle_pool; 

				bool thread_started;
				communication_queue_array_t* input_queue;
				communication_queue_array_t* output_queue;
				//to solve both-sending deadlock
				//we create a sender and a reciver
				pthread_t sender_thread;
				pthread_t receiver_thread;
				fd_queue_map_t fd_queue_map;
		};


		/*inline void communication_thread::set_threads_socket_map(threads_socket_map_t* threads_socket_map)
		{
			this->threads_socket_map=threads_socket_map;
		}*/

		inline void CBB_communication_thread::setup(communication_queue_array_t* input_queues,
				communication_queue_array_t* output_queues)
		{
			set_queues(input_queues, output_queues);
			//set_threads_socket_map(threads_socket_map);
		}
		inline int CBB_communication_thread::
			after_large_transfer(void * context, int mode)
		{
			//ignore;
			return SUCCESS;
		}
	}
}

#endif
