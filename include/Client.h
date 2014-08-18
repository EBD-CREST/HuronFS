
#ifndef CLIENT_H_
#define CLIENT_H_

class Client
{
protected:
	Client(int conn_port); 
	~client(); 
	int _connect_to_server(const struct sockaddr& client_addr, const struct sockaddr& server_addr) throw(std::runtime_error); 

private:
	struct sockaddr_in _client_addr;
	struct sockaddr_in _my_addr; 
	int conn_port; 

private:
	Client(const Client&); 
}

#endif
