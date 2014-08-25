#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/Client.h"
#include "include/IO_const.h"

int Client::_connect_to_server(struct sockaddr_in& client_addr, struct sockaddr_in& server_addr) throw(std::runtime_error)
{
	int client_socket = socket(PF_INET,   SOCK_STREAM,  0);  
	if( 0 > client_socket)
	{
		perror("Create Socket Failed");  
		throw std::runtime_error("Create Socket Failed");  
	}
	int on = 1; 
	setsockopt( client_socket,  SOL_SOCKET,  SO_REUSEADDR,  &on,  sizeof(on) ); 
	if(bind(client_socket,  reinterpret_cast<struct sockaddr*>(&client_addr),  sizeof(client_addr)))
	{
		perror("client bind port failed\n"); 
		throw std::runtime_error("client bind port failed\n"); 
	}
	int count=0; 
	while( MAX_CONNECT_TIME > ++count  &&  0 !=  connect(client_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr))); 
	if(MAX_CONNECT_TIME  ==  count)
	{
		close(client_socket); 
		perror("Can not Connect to Server");  
		throw std::runtime_error("Can not Connect to Server"); 
	}
	return client_socket; 
}
