#include <stdlib.h>
#include "CBB_task_parallel.h"

using namespace CBB::Common;
IO_task::IO_task():
	mode(SEND),
	socket(-1),
	message_size(0),
	basic_message(),
	extended_message_size(0),
	extended_message(NULL),
	next(NULL)
{}

/*task_parallel_queue::task_parallel_queue():
	queue_head(new task()),
	queue_tail(queue_head),
	queue_tmp_tail(queue_head),
	queue_tmp_head(queue_head),
	locker(),
	queue_empty()
{
	queue_head=new task();
	queue_tail->set_next(queue_head);
	queue_head->set_next(queue_tail);
	queue_tmp_head=queue_head;
	pthread_cond_init(&queue_empty, NULL);
	pthread_mutex_init(&locker, NULL);
}

task_parallel_queue::~task_parallel_queue()
{}

task* task_parallel_queue::get_task()
{
	while(queue_tail->get_next() == queue_head)
	{
		pthread_cond_wait(&queue_empty, &locker);
	}
	task* new_task=queue_tail->get_next();
	atomic_set(&queue_tail, queue_tail->get_next());
	return new_task;
}

task* task_parallel_queue::allocate_tmp_node()
{
	task* ret=NULL;
	if(queue_tmp_tail == queue_tmp_head->get_next())
	{
		ret=new task();
		ret->set_next(queue_head->get_next());
		queue_head->set_next(ret);
		queue_tmp_head=ret;
		ret=queue_head;
	}
	else
	{
		ret=queue_head;
		queue_tmp_head=queue_head->get_next();
		//atomic_set(&queue_head, queue_head->get_next());
	}

	return ret;
}

int task_parallel_queue::task_enqueue()
{
	task* original_head=queue_head;
	queue_head=queue_tmp_head;
	if(queue_tail->get_next() == original_head)
	{
		pthread_cond_signal(&queue_empty);
	}
}*/
