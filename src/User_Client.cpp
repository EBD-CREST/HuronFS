#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "include/IO_const.h"
#include "include/Communication.h"
#include "include/Query_Client.h"

Query_Client::Query_Client(const std::string& master_ip, int client_port)throw(std::runtime_error):
	Client()
{
	memset(&client_addr, 0, sizeof(client_addr)); 
	memset(&server_addr, 0, sizeof(server_addr)); 
	client_addr.sin_family = AF_INET; 
	client_addr.sin_addr.s_addr = htons(INADDR_ANY); 
	client_addr.sin_port = htons(client_port); 
	master_addr.sin_family = AF_INET; 
	master_addr.sin_port = htons(MASTER_CONN_PORT);
	if(0 == inet_aton(master_ip.c_str(), &master_addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error"); 
	}
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
	if(!strcmp(buffer, "open"))
	{
		query=OPEN_FILE; 
	}

	if(-1 == query)
	{
		_command(); 
	}
	else
	{
		int server_socket=Client::_connect_to_server(client_addr, master_addr); 
		Send(server_socket, query); 
		char path[100]; 
		scanf("%s", path); 
		Sendv(server_socket, path, strlen(path));
		Send(server_socket, O_RDONLY); 
		har *IOnode_info=Recvv(server_socket); 
		printf("%s", IOnode_info); 
		delete IOnode_info; 
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
