#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>

#include "Server.h"
#include "CBB_const.h"
#include "Communication.h"
#include "CBB_internal.h"

using namespace CBB::Common;

Server::Server(int port)throw(std::runtime_error):
	CBB_communication_thread(),
	CBB_request_handler(),
	CBB_remote_task(),
	_communication_input_queue(SERVER_THREAD_NUM),
	_communication_output_queue(SERVER_THREAD_NUM),
	_server_addr(sockaddr_in()), 
	_port(port), 
	_server_socket(-1)
{
	for(int i=0;i<SERVER_THREAD_NUM; ++i)
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
	CBB_communication_thread::set_queues(&_communication_output_queue, &_communication_input_queue);
	CBB_request_handler::set_queues(&_communication_input_queue, &_communication_output_queue);
	return; 
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
	}
	int id=0;
	Recv(socket, id);
	communication_queue_t& output_queue=output_queue_array->at(get_my_id());
	extended_IO_task* new_task=output_queue.allocate_tmp_node();
	new_task->set_receiver_id(id);
	CBB_communication_thread::receive_message(socket, new_task);
	_DEBUG("send to queue %p\n", &output_queue);
	output_queue.task_enqueue_signal_notification();
	return SUCCESS; 
}

int Server::input_from_producer(communication_queue_t* input_queue)
{
	while(!input_queue->is_empty())
	{
		extended_IO_task* new_task=input_queue->get_task();
		_DEBUG("new IO task\n");
		send(new_task);
		input_queue->task_dequeue();
	}
	return SUCCESS;
}

extended_IO_task* Server::init_response_task(extended_IO_task* input_task)
{
	communication_queue_t& output_queue=_communication_output_queue.at(get_my_id());
	extended_IO_task* output=output_queue.allocate_tmp_node();
	output->set_socket(input_task->get_socket());
	output->set_id_to_be_sent(input_task->get_receiver_id());
	return output;
}
