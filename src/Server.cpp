#include <cstring>
#include <unistd.h>
#include <stdio.h>

#include "include/Server.h"
#include "include/IO_const.h"

Server::Server(int port):
	_server_socket(0), 
	_server_addr(sockaddr_in()), 
	_port(port)
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
	if(0 != bind(_server_socket,  reinterpret_cast<struct sockaddr*>(&_server_addr),  sizeof(_server_addr)))
	{
		perror("Server Bind Port Failed");  
		throw std::runtime_error("Server Bind Port Failed");  
	}
	if(0 != listen(_server_socket,  MAX_QUEUE))
	{
		perror("Server Listen PORT ERROR");  
		throw std::runtime_error("Server Listen PORT ERROR");   
	}
	return; 
}

void Server::start_server()
{
	while(1)
	{
		struct sockaddr_in client_addr;  
		socklen_t length=sizeof(client_addr);  
		int new_client=accept(_server_socket,  reinterpret_cast<sockaddr *>(&client_addr),  &length);  
		if( 0 > new_client)
		{
			fprintf(stderr,  "Server Accept Failed\n");  
			close(new_client);  
			continue;  
		}
		fprintf(stderr,  "A New Client\n"); 
		_parse_request(new_client,  client_addr); 
		close(new_client);  
	}
	return;  
}

void Server::stop_server()
{
	close(_server_socket);  
	return;
}

