#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>

#include "include/Server.h"
#include "include/IO_const.h"
#include "include/Communication.h"

Server::Server(int port)throw(std::runtime_error):
	_server_addr(sockaddr_in()), 
	_port(port), 
	_epollfd(epoll_create(MAX_NODE_NUMBER+1))
{
	memset(&_server_addr, 0, sizeof(_server_addr)); 
	if(-1  ==  _epollfd)
	{
		perror("epoll_create"); 
		throw std::runtime_error("epoll_create"); 
	}
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
	struct epoll_event event;
	event.data.fd=_server_socket;
	event.events=EPOLLIN|EPOLLET;
	epoll_ctl(_epollfd, EPOLL_CTL_ADD, _server_socket, &event);
	return; 
}

int Server::_add_socket(int socketfd)
{
	struct epoll_event event; 
	event.data.fd=socketfd; 
	event.events=EPOLLIN|EPOLLET; 
	_IOnode_socket_pool.insert(socketfd); 
	return epoll_ctl(_epollfd, EPOLL_CTL_ADD, socketfd, &event); 
}

int Server::_delete_socket(int socketfd)
{
	struct epoll_event event; 
	event.data.fd=socketfd; 
	event.events=EPOLLIN|EPOLLET;
	_IOnode_socket_pool.erase(socketfd); 
	return epoll_ctl(_epollfd, EPOLL_CTL_DEL, socketfd, &event); 
}

void Server::start_server()
{
	struct epoll_event events[MAX_NODE_NUMBER+1]; 
	memset(events, 0, sizeof(struct epoll_event)*(MAX_NODE_NUMBER+1)); 
	while(1)
	{
		struct sockaddr_in client_addr;  
		socklen_t length=sizeof(client_addr);
		int nfds=epoll_wait(_epollfd, events, MAX_NODE_NUMBER+1, -1); 
		int ret=SUCCESS; 
		for(int i=0; i<nfds; ++i)
		{
			//new socket
			if(events[i].data.fd  == _server_socket)
			{
				int new_client=accept(_server_socket,  reinterpret_cast<sockaddr *>(&client_addr),  &length);  
				if( 0 > new_client)
				{
					fprintf(stderr,  "Server Accept Failed\n");  
					close(new_client);  
					continue;  
				}
				fprintf(stderr,  "A New Client\n"); 
				ret=_parse_new_request(new_client,  client_addr);
			}
			//communication from registed nodes
			else
			{
				ret=_parse_registed_request(events[i].data.fd); 
			}
		}
		if(SERVER_SHUT_DOWN == ret)
		{
			puts("I will shut down\n");
			break; 
		}
	}
	return;  
}

void Server::stop_server()
{
	for(socket_pool_t::iterator it=_IOnode_socket_pool.begin(); 
			it!=_IOnode_socket_pool.end(); ++it)
	{
		//inform each client that server is shutdown
		Send(*it, I_AM_SHUT_DOWN); 
		//close socket
		close(*it); 
	}
	close(_server_socket);  
	close(_epollfd); 
	return;
}

