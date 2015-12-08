#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Client.h"
#include "CBB_const.h"
#include "CBB_internal.h"

using namespace CBB::Common;

Client::Client():
	CBB_communication_thread(),
	CBB_connector(),
	_communication_input_queue(CLIENT_THREAD_NUM),
	_communication_output_queue(CLIENT_THREAD_NUM)
{
	for(int i=0; i<CLIENT_THREAD_NUM; ++i)
	{
		_communication_output_queue[i].set_queue_id(i);
		_communication_input_queue[i].set_queue_id(i);
	}
	CBB_communication_thread::set_queues(&_communication_output_queue, &_communication_input_queue);
}

Client::~Client()
{
	stop_client();
}

int Client::input_from_socket(int socket,
		communication_queue_array_t* output_queue_array)
{
	int id=0;
	Recv(socket, id);
	communication_queue_t& output_queue=output_queue_array->at(id);
	_DEBUG("output queue address %p\n",&output_queue);
	extended_IO_task* new_task=output_queue.allocate_tmp_node();
	new_task->set_receiver_id(id);
	CBB_communication_thread::receive_message(socket, new_task);
	output_queue.task_enqueue_signal_notification();
	return SUCCESS;
}

int Client::input_from_producer(communication_queue_t* input_queue)
{
	while(!input_queue->is_empty())
	{
		extended_IO_task* new_task=input_queue->get_task();
		_DEBUG("send request from producer\n");
		send(new_task);
		input_queue->task_dequeue();
	}
	return SUCCESS;
}

void Client::stop_client()
{
	CBB_communication_thread::stop_communication_server();
}

int Client::start_client()
{
	return CBB_communication_thread::start_communication_server();
}

communication_queue_t* Client::get_new_communication_queue()
{
	auto begin=_communication_output_queue.begin(), 
	     queue_ptr=begin,
	     end=_communication_output_queue.end();
	while(true)
	{
		if(0 == queue_ptr->try_lock_queue())
		{
			//return pointer
			return &(*queue_ptr);
		}
		else
		{
			++queue_ptr;
			if(end == queue_ptr)
			{
				queue_ptr=begin;
			}
		}
	}
	return nullptr;
}
