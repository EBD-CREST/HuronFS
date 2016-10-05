#include "CBB_heart_beat.h"
#include "CBB_internal.h"

using namespace CBB::Common;

CBB_heart_beat::CBB_heart_beat(communication_queue_t* input_queue,
		communication_queue_t* output_queue):
	keepAlive(NOT_KEEP_ALIVE),
	input_queue(input_queue),
	output_queue(output_queue)
{}

CBB_heart_beat::CBB_heart_beat():
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
