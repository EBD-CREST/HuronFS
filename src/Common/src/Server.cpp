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
	_communication_input_queue(),
	_communication_output_queue(),
	//keepAlive(KEEP_ALIVE),
	_server_addr(sockaddr_in()), 
	_port(port), 
	_server_socket(0)
{
	memset(&_server_addr, 0, sizeof(_server_addr)); 
}

Server::~Server()
{
	stop_server(); 
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
	CBB_communication_thread::_add_socket(_server_socket);
	CBB_communication_thread::set_queue(&_communication_input_queue, &_communication_output_queue);
	CBB_request_handler::set_queue(&_communication_output_queue, &_communication_input_queue);
	return; 
}

int Server::start_server()
{
	CBB_communication_thread::start_communication_server();
	return CBB_request_handler::start_handler();
}

void Server::stop_server()
{
	CBB_communication_thread::stop_communication_server();
	CBB_request_handler::stop_handler();
	close(_server_socket);  
	//close(_epollfd); 
	return;
}

int Server::_recv_real_path(extended_IO_task* new_task, std::string& real_path)
{
	char *path=NULL;
	new_task->pop_string(&path);
	real_path=_get_real_path(path);
	return SUCCESS;
}

int Server::_recv_real_relative_path(extended_IO_task* new_task, std::string& real_path, std::string& relative_path)
{
	char* path=NULL;
	//Recvv(clientfd, &path);
	new_task->pop_string(&path);
	relative_path=std::string(path);
	real_path=_get_real_path(relative_path);
	return SUCCESS;
}

int Server::input_from_socket(int socket, task_parallel_queue<extended_IO_task>* output_queue)
{
	extended_IO_task* new_task=output_queue->allocate_tmp_node();
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
		//ret=_parse_new_request(new_client,  client_addr);

	}
	CBB_communication_thread::receive_message(socket, new_task);
	output_queue->task_enqueue_signal_notification();
	return SUCCESS; 
}

int Server::input_from_producer(task_parallel_queue<extended_IO_task>* input_queue)
{
	while(!input_queue->is_empty())
	{
		extended_IO_task* new_task=input_queue->get_task();
		send(new_task);
		input_queue->task_dequeue();
	}
	return SUCCESS;
}
