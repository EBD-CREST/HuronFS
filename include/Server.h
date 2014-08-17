

#ifndef SERVER_H_
#define SERVER_H_

#include <netinet/in.h>
#include <stdexcept>

class Server
{
public:
	void stop_server(); 
	void start_server();
protected:
	Server(int port); 
	virtual ~Server(); 
	void _init_server()throw(std::runtime_error); 
	virtual void _parse_request(int, const struct sockaddr_in&)=0; 
private:
	int _server_socket; 
	struct sockaddr_in _server_addr;
	int _port; 

private:
	Server(const Server&); 
}; 

#endif
