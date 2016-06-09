#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <iterator>

#include "Server.h"
#include "CBB_const.h"
#include "Communication.h"
#include "CBB_internal.h"

using namespace CBB::Common;

Server::Server(int communication_thread_number, int port)throw(std::runtime_error):
	CBB_communication_thread(),
	CBB_request_handler(),
	CBB_remote_task(),
	_communication_input_queue(communication_thread_number),
	_communication_output_queue(communication_thread_number),
	_server_addr(sockaddr_in()), 
	_port(port), 
	_server_socket(-1),
	_threads_socket_map(communication_thread_number)
{
	for(int i=0;i<communication_thread_number; ++i)
	{
		_communication_output_queue[i].set_queue_id(i);
		_communication_input_queue[i].set_queue_id(i);
	}
	memset(&_server_addr, 0, sizeof(_server_addr)); 
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
	_setup_socket();
	CBB_communication_thread::setup(&_communication_output_queue,
			&_communication_input_queue);
	CBB_request_handler::set_queues(&_communication_input_queue, &_communication_output_queue);
	return; 
}

void Server::_setup_socket()
{
	memset(&_server_addr,  0,  sizeof(_server_addr)); 
	_server_addr.sin_family = AF_INET;  
	_server_addr.sin_addr.s_addr = htons(INADDR_ANY);  
	_server_addr.sin_port = htons(_port);  
	if( 0 > (_server_socket = socket(PF_INET,  SOCK_STREAM,  0)))
	{
		perror("Create Socket Failed");  
		throw std::runtime_error("Create Socket Failed");   
	}
	int on = 1; 
	setsockopt( _server_socket,  SOL_SOCKET,  SO_REUSEADDR,  &on,  sizeof(on) ); 

	struct timeval timeout;      
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	if (setsockopt (_server_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
				sizeof(timeout)) < 0)
	{
		perror("setsockopt failed");
		throw std::runtime_error("sockopt Failed");   
	}

	if (setsockopt (_server_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
				sizeof(timeout)) < 0)
	{
		perror("setsockopt failed");
		throw std::runtime_error("sockopt Failed");   
	}

	if(0 != bind(_server_socket,  reinterpret_cast<struct sockaddr*>(&_server_addr),  sizeof(_server_addr)))
	{
		perror("Server Bind Port Failed"); 
		fprintf(stderr, "port=%d\n", _port); 
		throw std::runtime_error("Server Bind Port Failed");  
	}
	if(0 != listen(_server_socket,  MAX_QUEUE))
	{
		perror("Server Listen PORT ERROR");  
		throw std::runtime_error("Server Listen PORT ERROR");   
	}
}

int Server::start_server()
{
	CBB_communication_thread::start_communication_server();
	CBB_remote_task::start_listening();
	CBB_communication_thread::_add_socket(_server_socket);
	return CBB_request_handler::start_handler();
}

void Server::stop_server()
{
	CBB_communication_thread::stop_communication_server();
	CBB_request_handler::stop_handler();
	CBB_remote_task::stop_listening();
	close(_server_socket);  
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

int Server::input_from_socket(int socket, communication_queue_array_t* output_queue_array)
{
	//new socket
	if(socket  == _server_socket)
	{
		struct sockaddr_in client_addr; 
		socklen_t length=sizeof(client_addr);
		socket=accept(_server_socket,  reinterpret_cast<sockaddr *>(&client_addr),  &length);  
		if( 0 > socket)
		{
			fprintf(stderr,  "Server Accept Failed\n");  
			close(socket); 
			return FAILURE; 
		}
		_DEBUG("A New Client\n"); 
		Server::_add_socket(socket);
	}
	else
	{
		int from_id=0, to_id=0;
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
			output_queue->task_enqueue_signal_notification();
		}
		catch(std::runtime_error &e)
		{
			//socket is killed after get to_id;
			//server won't get response;
			//since we allocated new task;
			//we deallocate it;
			if(nullptr != output_queue)
			{
				output_queue->putback_tmp_node();
			}
			node_failure_handler(socket);
		}
	}
	return SUCCESS; 
}

int Server::input_from_producer(communication_queue_t* input_queue)
{
	while(!input_queue->is_empty())
	{
		extended_IO_task* new_task=input_queue->get_task();
		_DEBUG("new IO task\n");
		try
		{
			send(new_task);
		}
		catch(std::runtime_error& e)
		{
			node_failure_handler(new_task->get_socket());
		}
		input_queue->task_dequeue();
	}
	return SUCCESS;
}

extended_IO_task* Server::init_response_task(extended_IO_task* input_task)
{
	communication_queue_t& output_queue=_communication_output_queue.at(input_task->get_id());
	extended_IO_task* output=output_queue.allocate_tmp_node();
	output->set_socket(input_task->get_socket());
	output->set_receiver_id(input_task->get_receiver_id());
	_threads_socket_map[input_task->get_id()]=input_task->get_socket();
	return output;
}

int Server::send_input_for_socket_error(int socket)
{
	communication_queue_t& input_queue=_communication_input_queue.at(0);
	extended_IO_task* input=input_queue.allocate_tmp_node();
	input->set_socket(socket);
	input->push_back(NODE_FAILURE);
	input_queue.task_enqueue_signal_notification();
	return SUCCESS;
}

communication_queue_t* Server::get_communication_queue_from_socket(int socket)
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

int Server::node_failure_handler(int node_socket)
{
	//dummy code
	_DEBUG("IOnode failed id=%d\n", node_socket);
	return send_input_for_socket_error(node_socket);
}
