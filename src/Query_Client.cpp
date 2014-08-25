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
	printf("c:close server\n"); 
}

void Query_Client::parse_query()
{
	char buffer[MAX_QUERY_LENGTH]; 
	scanf("%s", buffer); 
	if(-1  ==  server_addr.sin_port)
	{
		printf("set server first\n"); 
		return; 
	}
	if(!strcmp(buffer, "p"))
	{
		_print_node_info();
		return;
	}
	else if(!strcmp(buffer, "c"))
	{
		_server_shut_down();
		return;
	}
	else if(!strcmp(buffer, "f"))
	{
		_view_file_info();
		return;
	}
	_command(); 
}

void Query_Client::_print_node_info()
{
	int query=PRINT_NODE_INFO; 
	int server_socket=Client::_connect_to_server(client_addr, server_addr); 
	Send(server_socket, query); 
	int count=0;
	Recv(server_socket, count);
	char *ip=NULL;
	size_t total_memory, avaliable_memory;
	printf("%d IOnodes:\n", count);
	for(int i=0;i<count;++i)
	{
		Recvv(server_socket, &ip);
		Recv(server_socket, total_memory);
		Recv(server_socket, avaliable_memory);
		printf("IOnode %d\nip=%s\ntotal_memory=%lu\navaliable_memory=%lu\n", i+1, ip, total_memory, avaliable_memory);
		delete ip;
	}
	close(server_socket);
}

void Query_Client::_server_shut_down()
{
	int query=SERVER_SHUT_DOWN;
	int server_socket=Client::_connect_to_server(client_addr, server_addr); 
	Send(server_socket, query); 
	close(server_socket);
}

void Query_Client::_view_file_info()
{
	int query=GET_FILE_INFO;
	int server_socket=Client::_connect_to_server(client_addr, server_addr); 
	Send(server_socket, query);
	ssize_t file_no;
	scanf("%ld",&file_no);
	Send(server_socket, file_no);
	int ret=0;
	Recv(server_socket, ret);
	if(SUCCESS == ret)
	{
		size_t size, block_size;
		Recv(server_socket, size);
		Recv(server_socket, block_size);
		printf("file length=%lu\nblock_size=%lu\n", size, block_size);
		int count=0;
		Recv(server_socket, count);
		char *ip=NULL;
		off_t start_point;
		printf("%d IOnodes:\n", count);
		for(int i=0;i<count;++i)
		{
			Recv(server_socket, start_point);
			Recvv(server_socket, &ip);
			printf("start_point=%ld:\nip=%s\n",start_point, ip);
			delete ip;
		}
	}
	else
	{
		printf("File Not Found\n");
	}
	close(server_socket);
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
