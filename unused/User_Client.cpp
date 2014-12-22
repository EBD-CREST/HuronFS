#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "include/user_client.h"

User_Client::User_Client(const std::string& master_ip, int client_port)throw(std::runtime_error):
	Client()
{
	memset(&client_addr, 0, sizeof(client_addr)); 
	memset(&master_addr, 0, sizeof(master_addr)); 
	memset(&IOnode_addr, 0, sizeof(IOnode_addr)); 
	client_addr.sin_family = AF_INET; 
	client_addr.sin_addr.s_addr = htons(INADDR_ANY); 
	client_addr.sin_port = htons(client_port); 
	master_addr.sin_family = AF_INET; 
	master_addr.sin_port = htons(MASTER_PORT);
	IOnode_addr.sin_family = AF_INET; 
	IOnode_addr.sin_port = htons(IONODE_PORT);
	if(0 == inet_aton(master_ip.c_str(), &master_addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error"); 
	}
}

/*User_Client::~User_Client()
{
	for(file_point_t::iterator it=_opened_file.begin();
			it!=_opened_file.end();++it)
	{
		iob_close(it->first);
	}
}*/

