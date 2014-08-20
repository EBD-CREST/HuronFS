#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "include/IO_const.h"
#include "include/Communication.h"
#include "include/Query_Client.h"

Query_Client::Query_Client(int client_port):
	Client()
{
	memset(&client_addr, 0, sizeof(client_addr)); 
	memset(&server_addr, 0, sizeof(server_addr)); 
	client_addr.sin_family = AF_INET; 
	client_addr.sin_addr.s_addr = htons(INADDR_ANY); 
	client_addr.sin_port = htons(client_port); 
	server_addr.sin_family = AF_INET; 
	server_addr.sin_port = -1; 
}

void Query_Client::_command()
{
	printf("p:print IOnode info\n"); 
}

void Query_Client::parse_query()
{
	char buffer[MAX_QUERY_LENGTH]; 
	int query=-1; 
	printf("%s", buffer); 
	if(-1  ==  server_addr.sin_port)
	{
		printf("set server first\n"); 
		return; 
	}
	if(!strcmp(buffer, "p"))
	{
		query=PRINT_NODE_INFO; 
	}

	if(-1 == query)
	{
		_command(); 
	}
	else
	{
		int server_socket=Client::_connect_to_server(client_addr, server_addr); 
		Send(server_socket, query); 
		size_t length; 
		Recv(server_socket, length); 
		char *buffer=new char[length]; 
		Recvv(server_socket, buffer, length); 
		printf("%s", buffer); 
		delete buffer; 
	}
}

void Query_Client::set_server_addr(const std::string& ip, int port)throw(std::runtime_error)
{
	server_addr.sin_port = htons(port); 
	if(0  == inet_aton(ip.c_str(), &server_addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error"); 
	}
}
