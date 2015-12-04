#include "CBB_request_handler.h"

using namespace CBB::Common;

CBB_request_handler::CBB_request_handler(task_parallel_queue<extended_IO_task>* input_queue, task_parallel_queue<extended_IO_task>* output_queue):
	thread_started(UNSTARTED),
	handler_thread(),
	//init_barrier(),
	keepAlive(KEEP_ALIVE),
	input_queue(input_queue),
	output_queue(output_queue)
{}

CBB_request_handler::CBB_request_handler():
	thread_started(UNSTARTED),
	handler_thread(),
	//init_barrier(),
	keepAlive(KEEP_ALIVE),
	input_queue(nullptr),
	output_queue(nullptr)
{}

CBB_request_handler::~CBB_request_handler()
{
	void *ret=nullptr;
	if(STARTED == thread_started)
	{
		pthread_join(handler_thread, &ret);
	}
}

int CBB_request_handler::start_handler()
{
	if( nullptr == input_queue || nullptr == output_queue)
	{
		return FAILURE;
	}
	int ret=SUCCESS;;
	if(0 == (ret=pthread_create(&handler_thread, nullptr, handle_routine, static_cast<void*>(this))))
	{
		thread_started=STARTED;
	}
	return ret;
}

int CBB_request_handler::stop_handler()
{
	keepAlive = NOT_KEEP_ALIVE;
	void * ret=nullptr;
	return pthread_join(handler_thread, &ret);
}

void* CBB_request_handler::handle_routine(void* args)
{
	CBB_request_handler* this_obj=static_cast<CBB_request_handler*>(args);
	//pthread_barrier_wait(this_obj->init_barrier);

	while(KEEP_ALIVE == this_obj->keepAlive)
	{
		extended_IO_task* new_task=this_obj->input_queue->get_task();
		this_obj->_parse_request(new_task, this_obj->output_queue);
		this_obj->input_queue->task_dequeue();
	}
	_DEBUG("request handler thread end\n");
	return nullptr;
}

void CBB_request_handler::set_queue(task_parallel_queue<extended_IO_task>* input_queue, task_parallel_queue<extended_IO_task>* output_queue)
{
	this->input_queue=input_queue;
	this->output_queue=output_queue;
}
