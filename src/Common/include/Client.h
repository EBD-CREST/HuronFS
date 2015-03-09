
#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdexcept>

class Client
{
public:
	virtual ~Client();
protected:
	int _connect_to_server(const struct sockaddr_in& client_addr, const struct sockaddr_in& server_addr)const throw(std::runtime_error); 
}; 

#endif
