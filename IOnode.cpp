/*
 * IOnode.c
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */

#include "IOnode.h"
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

IOnode::block::block(unsigned long size, int block_id) throw(std::bad_alloc):size(size),block_id(block_id),data(NULL)
{
	data=malloc(size);
	if( NULL == data)
	{
	   throw std::bad_alloc(); 
	}
	return;
}

IOnode::block::~block()
{
	free(data);
}

IOnode::block::block(const block & src):size(src.size),block_id(src.block_id),data(src.data){};

IOnode::IOnode(std::string my_ip, std::string master_ip,  int master_port) throw(std::runtime_error):
	ip(my_ip),
	node_id(-1),
	_blocks(std::map<int, std::vector<class block> >()), 
	_current_block_number(0), 
	MAX_BLOCK_NUMBER(MAX_BLOCK_NUMBER_), 
	_memory(MEMORY), 
	port(MASTER_PORT), 
	node_server_socket(-1), 
	master_socket(-1) 
{
	if(-1  ==  (node_id=regist(master_ip,  master_port)))
	{
		throw std::runtime_error("Get Node Id Error"); 
	}
	try
	{
		start_server(); 
	}
	catch(std::runtime_error& e)
	{
		unregist();
		throw;
	}
};

void IOnode::start_server() throw(std::runtime_error)
{
	memset(&node_server_addr, 0, sizeof(node_server_addr));
	node_server_addr.sin_family = AF_INET; 
	node_server_addr.sin_addr.s_addr = htons(INADDR_ANY); 
	node_server_addr.sin_port = htons(port); 
	if( 0 > (node_server_socket = socket(PF_INET, SOCK_STREAM, 0)))
	{
		throw std::runtime_error("Create Socket Failed");  
	}
	if(0 != bind(node_server_socket, (struct sockaddr*)&node_server_addr, sizeof(node_server_addr)))
	{
		throw std::runtime_error("Server Bind Port Failed"); 
	}
	if(0 != listen(node_server_socket, MAX_QUEUE))
	{
		throw std::runtime_error("Server Listen PORT ERROR");  
	}
	
	std::cout << "Start IO node Server" << std::endl; 
}	

IOnode::~IOnode()
{
	if(-1 != node_server_socket)
	{
		close(node_server_socket); 
	}
	unregist(); 
	delete &_blocks; 
}

int IOnode::regist(std::string& master_ip, int master_port) throw(std::runtime_error)
{ 
	memset(&master_addr, 0, sizeof(master_addr)); 
	master_conn_addr.sin_family = AF_INET;
	master_conn_addr.sin_addr.s_addr = htons(MASTER_CONN_PORT); 
	
	master_socket = socket(PF_INET,  SOCK_STREAM, 0); 
	if( 0 > master_socket)
	{
		//perror("Create Socket Failed"); 
		throw std::runtime_error("Create Socket Failed"); 
	}

	if(bind(master_socket,  (struct sockaddr*)&master_conn_addr, sizeof(master_conn_addr)))
	{
		//perror("Client Bind Port Failed");
		throw std::runtime_error("Client Bind Port Failed");  
	}
	memset(&master_addr, 0,  sizeof(master_addr)); 
	master_addr.sin_family = AF_INET; 
	if( 0  ==  inet_aton(master_ip.c_str(), &master_addr.sin_addr))
	{
		//perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error");
	}

	master_addr.sin_port = htons(master_port); 
	if( connect(master_socket,  (struct sockaddr*)&master_addr,  sizeof(master_addr)))
	{
		throw std::runtime_error("Can Not Connect To master");  
	}
	send(master_socket, &REGIST, sizeof(REGIST),  0);
	char id[sizeof(int)]; 
	recv(master_socket, id,  sizeof(id), 0);
	return atoi(id); 
}

void IOnode::unregist()
{
	if(-1 != master_socket)
	{
		send(master_socket,  &UNREGIST,  sizeof(UNREGIST),  0);
		close(master_socket); 
		master_socket = -1; 
	}
}
