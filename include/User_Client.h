#ifndef QUERY_CLIENT_H_
#define QUERY_CLIENT_H_

#include <sys/socket.h>
#include <netinet/in.h>

#include "include/Client.h"

class User_Client:public Client
{
public:
	explicit User_Client(int port); 
	int open(const std::string path, int flag); 
	ssize_t iob_read(int fd, void *buffer, size_t count); 
	ssize_t iob_write(int fd, const void *buffer, size_t count); 

private:
	User_Client(const User_Client&); 

private:
	struct sockaddr_in server_addr; 
	struct sockaddr_in client_addr; 
};

#endif
