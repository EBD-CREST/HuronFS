
#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdexcept>

class Client
{
protected:
	int _connect_to_server(struct sockaddr_in& client_addr, struct sockaddr_in& server_addr)throw(std::runtime_error); 
}; 

#endif
