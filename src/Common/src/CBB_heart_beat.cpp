#include "CBB_heart_beat.h"

using namespace CBB::Common;

CBB_heart_beat::CBB_heart_beat():
	input_queue(nullptr),
	output_queue(nullptr)
{}

void CBB_heart_beat::set_task_parallel_queue(communication_queue_t* input_queue,
		communication_queue_t* output_queue)
{
	this->input_queue=input_queue;
	this->output_queue=output_queue;
}

int CBB_heart_beat::heart_beat_check()
{
	socket_map_t map;
	get_IOnode_socket_map(map);
	for(const auto& item : map)
	{
		if(SUCCESS != send_heart_beat_check(item.second))
		{
			failure_headler(item.first);
		}
	}
	return SUCCESS;
}

void CBB_heart_beat::set_task_parallel_queue(communication_queue_t* input_queue,
		communication_queue_t* output_queue)
{
	this->input_queue=input_queue;
	this->output_queue=output_queue;
}

int CBB_heart_beat::send_heart_beat_check(int socket)
{
	int ret=0;
	extended_IO_task* new_task=output_queue->allocate_tmp_node();
	new_task->push_back(HEART_BEAT);
	extended_IO_task* output_queue->task_enqueue();
	extended_IO_task* response=input_queue->get_task();
	response->pop(ret);
	input_queue->task_dequeue();
	return ret;
}
