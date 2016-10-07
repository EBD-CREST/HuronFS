#include "Comm_basic.h"

using namespace std;
using namespace CBB::Common;

int Comm_basic::
	create_socket(const struct sockaddr_in& addr)
throw(std::runtime_error)
{
	int 		sockfd = 0;
	int 		on     = 1; 
	struct timeval  timeout;      
	
	set_timer(&timeout);

	if( 0 > (sockfd = socket(PF_INET,  SOCK_STREAM,  0)))
	{
		perror("Create Socket Failed");  
		throw std::runtime_error("Create Socket Failed");   
	}

	if (setsockopt(sockfd,  SOL_SOCKET,
				SO_REUSEADDR,  &on,  sizeof(on) ))
	{
		perror("setsockopt failed");
		throw std::runtime_error("sockopt Failed");   
	}

	if (setsockopt(sockfd, SOL_SOCKET,
				SO_RCVTIMEO, (char *)&timeout,
				sizeof(timeout)) < 0)
	{
		perror("setsockopt failed");
		throw std::runtime_error("sockopt Failed");   
	}

	if(0 != bind(sockfd, 
				reinterpret_cast<const struct sockaddr*>(&addr),
				sizeof(addr)))
	{
		perror("Server Bind Port Failed"); 
		throw std::runtime_error("Server Bind Port Failed");  
	}

	return sockfd;
}

