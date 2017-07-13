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

#include "CBB_data_sync.h"

using namespace std;
using namespace CBB;
using namespace CBB::Common;

data_sync_task::
data_sync_task(int id, data_sync_task* next):
	//base class
	basic_task(id, next),
	//fields
	task_id(),
	handle(),
	receiver_id(),
	_file(),
	start_point(),
	offset(),
	size()
{}

CBB_data_sync::CBB_data_sync():
	//fields
	keepAlive(KEEP_ALIVE),
	thread_started(UNSTARTED),
	pthread_id(-1),
	data_sync_queue(),
	communication_input_queue_ptr(nullptr),
	communication_output_queue_ptr(nullptr)
{}

CBB_data_sync::~CBB_data_sync()
{
	stop_listening();
}

int CBB_data_sync::start_listening()
{
	int ret=0;
	if(0 == (ret=pthread_create(&pthread_id, nullptr, data_sync_thread_fun, this)))
	{
		thread_started=STARTED;
	}
	else
	{
		perror("pthread_create");
	}
	return ret;
}

void CBB_data_sync::stop_listening()
{
	keepAlive=NOT_KEEP_ALIVE;
	void* ret=nullptr;
	if(STARTED == thread_started)
	{
		pthread_join(pthread_id, &ret);
		//pthread_join(queue_event_wait_thread, &ret);
		thread_started = UNSTARTED;
	}
	return;
}

void* CBB_data_sync::
data_sync_thread_fun(void* args)
{
	CBB_data_sync* this_obj=static_cast<CBB_data_sync*>(args);

	while(KEEP_ALIVE == this_obj->keepAlive)
	{
		data_sync_task* new_task=this_obj->data_sync_queue.get_task();
		_DEBUG("data sync parser received\n");
		this_obj->data_sync_parser(new_task);
		this_obj->data_sync_queue.task_dequeue();
	}
	_DEBUG("request handler thread end\n");
	return nullptr;
}

void CBB_data_sync::set_queues(communication_queue_t* input_queue,
		communication_queue_t* output_queue)
{
	this->communication_input_queue_ptr=input_queue;
	this->communication_output_queue_ptr=output_queue;
}

int CBB_data_sync::add_data_sync_task(int task_id,
		void* file,
		off64_t start_point,
		off64_t offset,
		ssize_t size,
		int receiver_id,
		comm_handle_t handle,
		send_buffer_t* send_buffer)
{
	data_sync_task* new_data_sync_task=data_sync_queue.allocate_tmp_node();
	new_data_sync_task->task_id=task_id;
	new_data_sync_task->_file=file;
	new_data_sync_task->start_point=start_point;
	new_data_sync_task->receiver_id=receiver_id;
	new_data_sync_task->offset=offset;
	new_data_sync_task->size=size;
	new_data_sync_task->set_handle(handle);
	new_data_sync_task->set_send_buffer(send_buffer);
	return data_sync_queue.task_enqueue_signal_notification();
}
