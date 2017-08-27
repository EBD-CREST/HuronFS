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

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "CBB_remote_task.h"
#include "CBB_const.h"

using namespace CBB::Common;

remote_task::remote_task():
	//base class
	basic_task(),
	//fields
	task_id(0),
	task_data(nullptr),
	extended_task_data(nullptr)
{}

remote_task::remote_task(int id, remote_task* next):
	//base class
	basic_task(id, next),
	//fields
	task_id(0),
	task_data(nullptr),
	extended_task_data(nullptr)
{}

CBB_remote_task::CBB_remote_task():
	CBB_basic_thread(CBB_REMOTE_TASK_THREAD_NUMBER),
	thread_started(UNSTARTED),
	remote_task_queue(0),
	locker()
{}

CBB_remote_task::~CBB_remote_task()
{
	stop_listening();
}

int CBB_remote_task::start_listening()
{
	return create_thread(thread_fun, this);
}

void CBB_remote_task::stop_listening()
{
	if(STARTED == thread_started)
	{
		end_thread();
		thread_started = UNSTARTED;
	}
	return;
}

void* CBB_remote_task::thread_fun(void* args)
{
	CBB_remote_task* this_obj=static_cast<CBB_remote_task*>(args);

	while(this_obj->keepAlive())
	{
		remote_task* new_task=this_obj->remote_task_queue.get_task();
		_DEBUG("remote task received\n");
		this_obj->remote_task_handler(new_task);
		this_obj->remote_task_queue.task_dequeue();
	}
	_DEBUG("request handler thread end\n");
	return nullptr;
}

remote_task* CBB_remote_task::add_remote_task(int task_code, void* task_data)
{
	remote_task* new_task = remote_task_queue.allocate_tmp_node();
	new_task->set_task_id(task_code);
	new_task->set_task_data(task_data);
	remote_task_queue.task_enqueue_signal_notification();
	return new_task;
}

remote_task* CBB_remote_task::add_remote_task(int task_code, void* task_data, void* extended_task_data)
{
	remote_task* new_task=add_remote_task(task_code, task_data);
	new_task->set_extended_task_data(extended_task_data);
	return new_task;
}
