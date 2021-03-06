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

#ifndef CBB_DATA_SYNC_H_
#define CBB_DATA_SYNC_H_

#include "CBB_task_parallel.h"
#include "CBB_communication_thread.h"
#include "CBB_IO_task.h"
#include "CBB_basic_thread.h"

namespace CBB
{
	namespace Common
	{
		class data_sync_task: public basic_task
		{
			public:
				data_sync_task(int id, data_sync_task* next);
				data_sync_task()=default;
				virtual ~data_sync_task()=default;
				void set_handle(comm_handle_t handle);
				void set_send_buffer(send_buffer_t* send_buffer);
				send_buffer_t* get_send_buffer();

				int 		task_id;
				comm_handle 	handle;
				int 		receiver_id;
				void* 		_file;
				off64_t 	start_point;
				off64_t 	offset;
				ssize_t 	size;
				send_buffer_t 	send_buffer;
		};
		typedef task_parallel_queue<data_sync_task> data_sync_queue_t;

		class CBB_data_sync:
			public CBB_basic_thread
		{
			public:
				CBB_data_sync();
				virtual ~CBB_data_sync();
				int start_listening();
				void stop_listening();
				
				int add_data_sync_task(int task_id,
						void* file,
						off64_t start_point,
						off64_t offset,
						ssize_t size,
						int receiver_id,
						comm_handle_t handle,
						send_buffer_t* send_buffer);
				static void* data_sync_thread_fun(void* argv);
				void set_queues(communication_queue_t* input_queue,
						communication_queue_t* output_queue);
				virtual int data_sync_parser(data_sync_task* new_task)=0;
				extended_IO_task* allocate_data_sync_task();
				int data_sync_task_enqueue(extended_IO_task* output_task);
				extended_IO_task* get_data_sync_response();
				extended_IO_task* data_sync_response_dequeue(extended_IO_task* response);

			private:
				int 			thread_started;
				data_sync_queue_t 	data_sync_queue;
				communication_queue_t* 	communication_input_queue_ptr;
				communication_queue_t* 	communication_output_queue_ptr;
		};

		inline extended_IO_task* CBB_data_sync::allocate_data_sync_task()
		{
			extended_IO_task* ret=communication_output_queue_ptr->allocate_tmp_node();
			_DEBUG("data sync element %p\n", ret);
			return ret;
		}

		inline int CBB_data_sync::data_sync_task_enqueue(extended_IO_task* output_task)
		{
			_DEBUG("task enqueue %p\n", communication_output_queue_ptr);
			return communication_output_queue_ptr->task_enqueue();
		}

		inline extended_IO_task* CBB_data_sync::get_data_sync_response()
		{
			return communication_input_queue_ptr->get_task();
		}

		inline extended_IO_task* CBB_data_sync::data_sync_response_dequeue(extended_IO_task* response)
		{
			return communication_input_queue_ptr->task_dequeue();
		}

		inline void data_sync_task::
			set_handle(comm_handle_t handle)
		{
			this->handle=*handle;
		}

		inline void data_sync_task::
			set_send_buffer(send_buffer_t* send_buffer)
		{
			if(nullptr != send_buffer)
			{
				this->send_buffer.swap(*send_buffer);
			}
		}

		inline send_buffer_t* data_sync_task::
			get_send_buffer()
		{
			return &this->send_buffer;
		}
	}
}
#endif
