#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iterator>

#include "Client.h"
#include "CBB_const.h"
#include "CBB_internal.h"

using namespace CBB::Common;
using namespace std;

Client::Client(int communication_thread_number):
	CBB_communication_thread(),
	CBB_connector(),
	_communication_input_queue(communication_thread_number),
	_communication_output_queue(communication_thread_number),
	_threads_socket_map(communication_thread_number)
{
	for(int i=0; i<communication_thread_number; ++i)
	{
		_communication_output_queue[i].set_queue_id(i);
		_communication_input_queue[i].set_queue_id(i);
	}
	CBB_communication_thread::setup(&_communication_output_queue,
			&_communication_input_queue);
}

Client::~Client()
{
	stop_client();
}

int Client::input_from_socket(int socket,
		communication_queue_array_t* output_queue_array)
{
	int to_id=0, from_id=0;
	communication_queue_t* output_queue=nullptr;
	extended_IO_task* new_task=nullptr;
	try
	{
		Recv(socket, from_id);
		Recv(socket, to_id);
		output_queue=&output_queue_array->at(to_id);
		new_task=output_queue->allocate_tmp_node();
		new_task->set_receiver_id(from_id);
		CBB_communication_thread::receive_message(socket, new_task);
	}
	catch(std::runtime_error &e)
	{
		//socket is killed before getting to_id;
		//we return an error code;
		if(nullptr == output_queue)
		{
			output_queue=get_communication_queue_from_socket(socket);
			new_task=output_queue->allocate_tmp_node();
		}
		new_task->set_error(SOCKET_KILLED);
		node_failure_handler(socket);
	}
	output_queue->task_enqueue_signal_notification();
	return SUCCESS;
}

int Client::input_from_producer(communication_queue_t* input_queue)
{
	while(!input_queue->is_empty())
	{
		extended_IO_task* new_task=input_queue->get_task();
		_DEBUG("send request from producer\n");
		try
		{
			send(new_task);
		}
		catch(std::runtime_error& e)
		{
			//for client if server died before data is sent;
			//we return an error code to client;
			reply_with_socket_error(new_task);
			node_failure_handler(new_task->get_socket());
		}
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

communication_queue_t* Client::get_communication_queue_from_socket(int socket)
{
	for(threads_socket_map_t::iterator it=begin(_threads_socket_map);
			end(_threads_socket_map) != it; ++it)
	{
		if(socket == *it)
		{
			return &_communication_input_queue.at(distance(begin(_threads_socket_map), it));
		}
	}
	return nullptr;
}

int Client::reply_with_socket_error(extended_IO_task* input_task)
{
	int index=input_task->get_id();
	communication_queue_t& input_queue=_communication_input_queue.at(index);
	extended_IO_task* new_task=input_queue.allocate_tmp_node();
	new_task->set_error(SOCKET_KILLED);
	input_queue.task_enqueue_signal_notification();
	return SUCCESS;
}

int Client::node_failure_handler(int node_socket)
{
	//dummy code
	_DEBUG("IOnode failed id=%d\n", node_socket);
	return SUCCESS;
}
