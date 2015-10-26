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
	_communication_input_queue(),
	_communication_output_queue()
{
	CBB_communication_thread::set_queue(&_communication_output_queue, &_communication_input_queue);
}

Client::~Client()
{
	stop_client();
}

int Client::input_from_socket(int socket,
		task_parallel_queue<extended_IO_task>* output_queue)
{
	extended_IO_task* new_task=output_queue->allocate_tmp_node();
	receive_message(socket, new_task);
	output_queue->task_enqueue_signal_notification();
	return SUCCESS;
}

int Client::input_from_producer(task_parallel_queue<extended_IO_task>* input_queue)
{
	while(!input_queue->is_empty())
	{
		extended_IO_task* new_task=input_queue->get_task();
		_DEBUG("request from producer\n");
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
