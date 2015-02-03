#ifndef QUERY_CLIENT_H_
#define QUERY_CLIENT_H_

#include <sys/socket.h>
#include <netinet/in.h>

#include "include/Client.h"

class Query_Client:public Client
{
public:
	explicit Query_Client(int port); 
	void parse_query();
	void set_server_addr(const std::string& ip, int port)throw(std::runtime_error); 

private:
	Query_Client(const Query_Client&); 
	void _print_node_info();
	void _server_shut_down();
	void _view_file_info();
	void _command();

private:
	struct sockaddr_in server_addr; 
	struct sockaddr_in client_addr; 
};

#endif
