#ifndef QUERY_CLIENT_H_
#define QUERY_CLIENT_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <map>

#include "include/Client.h"

class User_Client:public Client
{
public:
	User_Client(const std::string &master_ip, int port)throw(std::runtime_error); 
	void parse_query();
private:
	User_Client(const User_Client&); 
	void _command();
	void _set_ip(const std;;string &ip);
private:

	struct sockaddr_in master_addr; 
	struct sockaddr_in IOnode_addr;
	struct sockaddr_in client_addr; 
};


#endif
