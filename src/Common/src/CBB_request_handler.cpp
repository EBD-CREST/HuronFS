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

#include "CBB_request_handler.h"

using namespace CBB::Common;

CBB_request_handler::
CBB_request_handler(	communication_queue_array_t* input_queue,
			communication_queue_array_t* output_queue):
	CBB_basic_thread(CBB_REQUEST_HANDLER_THREAD_NUMBER),
	thread_started(UNSTARTED),
	input_queue(input_queue),
	output_queue(output_queue)
{}

CBB_request_handler::
CBB_request_handler():
	CBB_basic_thread(CBB_REQUEST_HANDLER_THREAD_NUMBER),
	thread_started(UNSTARTED),
	input_queue(nullptr),
	output_queue(nullptr)
{}

CBB_request_handler::~CBB_request_handler()
{
	stop_handler();
}

int CBB_request_handler::start_handler()
{
	if( nullptr == input_queue || nullptr == output_queue)
	{
		return FAILURE;
	}
	int ret=SUCCESS;;
	if(SUCCESS == (ret=create_thread(handle_routine, this)))
	{
		thread_started=STARTED;
	}
	return ret;
}

int CBB_request_handler::stop_handler()
{
	
	if(STARTED == thread_started)
	{
		return end_thread();
	}
	else
	{
		return SUCCESS;
	}
}

void* CBB_request_handler::
handle_routine(void* args)
{
	CBB_request_handler* this_obj=static_cast<CBB_request_handler*>(args);
	communication_queue_t& input_queue=this_obj->input_queue->at(0);
	//communication_queue_t& output_queue=this_obj->output_queue->at(id);

	while(this_obj->keepAlive())
	{
		this_obj->interval_process();

		extended_IO_task* new_task=input_queue.get_task();
		_DEBUG("new task element %p\n", new_task);
		this_obj->_parse_request(new_task);
		if(new_task != input_queue.task_dequeue())
		{
			_LOG("DEQUEUE ERROR\n");
		}

		_DEBUG("task ends\n");
	}
	_DEBUG("request handler thread end\n");
	return nullptr;
}

void CBB_request_handler::set_queues(communication_queue_array_t* input_queue,
		communication_queue_array_t* output_queue)
{
	this->input_queue=input_queue;
	this->output_queue=output_queue;
}
