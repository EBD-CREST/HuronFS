#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <iterator>

#include "Server.h"
#include "CBB_const.h"
#include "Comm_api.h"
#include "CBB_internal.h"

using namespace CBB::Common;

Server::
Server(int communication_thread_number, int port)throw(std::runtime_error):
	CBB_communication_thread(),
	CBB_request_handler(),
	CBB_remote_task(),
	_communication_input_queue(communication_thread_number),
	_communication_output_queue(communication_thread_number),
	_server_handle(),
	server_port(port),
	_threads_handle_map(communication_thread_number)
{
	for(int i=0;i<communication_thread_number; ++i)
	{
		_communication_output_queue[i].set_queue_id(i);
		_communication_input_queue[i].set_queue_id(i);
	}
}

Server::~Server()
{
	stop_server(); 
	for(auto& queue:_communication_output_queue)
	{
		queue.destory_queue();
	}
	for(auto& queue:_communication_input_queue)
	{
		queue.destory_queue();
	}
}

void Server::_init_server()throw(std::runtime_error)
{
	configure_dump();
	CBB_communication_thread::setup(&_communication_output_queue,
			&_communication_input_queue);
	CBB_request_handler::set_queues(&_communication_input_queue, &_communication_output_queue);
	return; 
}

void Server::_setup_server()
{
	init_server_handle(_server_handle, this->server_port);
}

int Server::start_server()
{
	init_protocol();
	_setup_server();
	CBB_communication_thread::start_communication_server();
	CBB_remote_task::start_listening();
	CBB_communication_thread::_add_handle(&_server_handle);
	return CBB_request_handler::start_handler();
}

void Server::stop_server()
{
	CBB_communication_thread::stop_communication_server();
	CBB_request_handler::stop_handler();
	CBB_remote_task::stop_listening();
	Close(&_server_handle);  
	return;
}

int Server::_recv_real_path(extended_IO_task* new_task, std::string& real_path)
{
	char *path=nullptr;
	new_task->pop_string(&path);
	real_path=_get_real_path(path);
	return SUCCESS;
}

int Server::_recv_real_relative_path(extended_IO_task* new_task, std::string& real_path, std::string& relative_path)
{
	char* path=nullptr;
	new_task->pop_string(&path);
	relative_path=std::string(path);
	real_path=_get_real_path(relative_path);
	return SUCCESS;
}

int Server::
input_from_network(comm_handle_t handle,
		communication_queue_array_t* output_queue_array)
{
	//new handle
#ifdef TCP
	if( compare_handle(&_server_handle, handle))
	{
		struct sockaddr_in client_addr; 
		socklen_t length=sizeof(client_addr);
		int socket=accept(_server_handle.socket,  reinterpret_cast<sockaddr *>(&client_addr),  &length);  
		if( 0 > socket)
		{
			fprintf(stderr,  "Server Accept Failed\n");  
			Close(handle); 
			return FAILURE; 
		}
		_DEBUG("A New Client\n"); 
		comm_handle new_handle;
		new_handle.socket=socket;
		Server::_add_handle(&new_handle);
	}
	else
	{
#endif
		int to_id=0;
		communication_queue_t* output_queue=nullptr;
		extended_IO_task* new_task=nullptr;
		try
		{
			Recv(handle, to_id);
			output_queue=&output_queue_array->at(to_id);
			_DEBUG("to id=%d\n", to_id);
			new_task=output_queue->allocate_tmp_node();
			new_task->set_receiver_id(to_id);
			CBB_communication_thread::receive_message(handle, new_task);
			output_queue->task_enqueue_signal_notification();
		}
		catch(std::runtime_error &e)
		{
			//handle is killed after get to_id;
			//server won't get response;
			//since we allocated new task;
			//we deallocate it;
			if(nullptr != output_queue)
			{
				output_queue->putback_tmp_node();
			}
			node_failure_handler(handle);
		}
#ifdef TCP
	}
#endif
	return SUCCESS; 
}

int Server::input_from_producer(communication_queue_t* input_queue)
{
	while(!input_queue->is_empty())
	{
		extended_IO_task* new_task=input_queue->get_task();
		_DEBUG("new IO task\n");
		if(new_task->is_new_connection())
		{
			comm_handle handle;

			Connect(new_task->get_uri(),
					new_task->get_port(),
					handle, 
					new_task->get_message(),
					reinterpret_cast<size_t*>(
						new_task->get_message()+MESSAGE_SIZE_OFF));

			Server::_add_handle(&handle);
		}
		else
		{
			try
			{
#ifdef CCI
				if(0 != new_task->get_extended_data_size())
				{
					send_rma(new_task);
				}
				else
				{
#endif
					send(new_task);
#ifdef CCI
				}
#endif
			}
			catch(std::runtime_error& e)
			{
				node_failure_handler(new_task->get_handle());
			}
		}
		input_queue->task_dequeue();
	}
	return SUCCESS;
}

extended_IO_task* Server::init_response_task(extended_IO_task* input_task)
{
	communication_queue_t& output_queue=_communication_output_queue.at(input_task->get_id());
	extended_IO_task* output=output_queue.allocate_tmp_node();
	output->set_handle(input_task->get_handle());
	output->set_receiver_id(input_task->get_receiver_id());
	_threads_handle_map[input_task->get_id()]=input_task->get_handle();
	return output;
}

int Server::send_input_for_handle_error(comm_handle_t handle)
{
	communication_queue_t& input_queue=_communication_input_queue.at(0);
	extended_IO_task* input=input_queue.allocate_tmp_node();
	input->set_handle(handle);
	input->push_back(NODE_FAILURE);
	input_queue.task_enqueue_signal_notification();
	return SUCCESS;
}

communication_queue_t* Server::get_communication_queue_from_handle(comm_handle_t handle)
{
	for(threads_handle_map_t::iterator it=begin(_threads_handle_map);
			end(_threads_handle_map) != it; ++it)
	{
		if(handle == *it)
		{
			return &_communication_input_queue.at(distance(begin(_threads_handle_map), it));
		}
	}
	return nullptr;
}

int Server::node_failure_handler(comm_handle_t node_handle)
{
	//dummy code
	//_DEBUG("IOnode failed id=%d\n", node_handle);
	return send_input_for_handle_error(node_handle);
}
