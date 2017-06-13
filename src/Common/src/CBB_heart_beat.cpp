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

#include "CBB_heart_beat.h"
#include "CBB_internal.h"

using namespace CBB::Common;

CBB_heart_beat::CBB_heart_beat(communication_queue_t* input_queue,
		communication_queue_t* output_queue):
	//fields
	keepAlive(NOT_KEEP_ALIVE),
	input_queue(input_queue),
	output_queue(output_queue)
{}

CBB_heart_beat::CBB_heart_beat():
	//fields
	keepAlive(NOT_KEEP_ALIVE),
	input_queue(nullptr),
	output_queue(nullptr)
{}

void CBB_heart_beat::set_queues(communication_queue_t* input_queue,
		communication_queue_t* output_queue)
{
	this->input_queue=input_queue;
	this->output_queue=output_queue;
}

int CBB_heart_beat::heart_beat_check()
{
	_DEBUG("start heart beat check\n");
	handle_map_t map;
	get_IOnode_handle_map(map);
	for(const auto& item : map)
	{
		send_heart_beat_check(item.first);
	}
	return SUCCESS;
}

/*void CBB_heart_beat::start_heart_beat_check()
{
	keepAlive=KEEP_ALIVE;
	while(KEEP_ALIVE == keepAlive)
	{
		heart_beat_check();
		sleep(HEART_BEAT_INTERVAL);
	}
	return;
}

void CBB_heart_beat::stop_heart_beat_check()
{
	keepAlive=NOT_KEEP_ALIVE;
}*/

int CBB_heart_beat::send_heart_beat_check(comm_handle_t handle)
{
	int ret=0;
	extended_IO_task* new_task=output_queue->allocate_tmp_node();
	new_task->set_handle(handle);
	new_task->push_back(HEART_BEAT);
	output_queue->task_enqueue();
	return ret;
}
