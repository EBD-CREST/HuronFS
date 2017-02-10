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

#ifndef CBB_REQUEST_HANDLER_H_
#define CBB_REQUEST_HANDLER_H_

#include <pthread.h>
#include "CBB_task_parallel.h"
#include "CBB_communication_thread.h"

namespace CBB
{
	namespace Common
	{

		class CBB_request_handler
		{
			public:
				CBB_request_handler(communication_queue_array_t* input_queue,
						communication_queue_array_t* output_queue);
				CBB_request_handler();
				virtual ~CBB_request_handler();
				virtual int _parse_request(extended_IO_task* new_task)=0; 
				int start_handler();
				int stop_handler();
				static void* handle_routine(void *arg);

				void set_queues(communication_queue_array_t* input_queue,
						communication_queue_array_t* output_queue);

			private:
				bool thread_started;
				pthread_t handler_thread;
				int keepAlive;
				communication_queue_array_t* input_queue;
				communication_queue_array_t* output_queue;

		};
	}
}

#endif
