/*
 * Copyright (c) 2017, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. 
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp, LLNL-CODE-722817.
 * All rights reserved. 
 *
 * This file is part of HuronFS.
 *
 * Please also read the file "LICENSE" included in this package for 
 * Our Notice and GNU Lesser General Public License.
 *
 * This program is free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License (as published by the Free Software 
 * Foundation) version 2.1 dated February 1999. 
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
 * General Public License for more details. 
 *
 * You should have received a copy of the GNU Lesser General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "comm_type/tcp.h"

using namespace std;
using namespace CBB;
using namespace CBB::Common;

CBB_error CBB_tcp::
init_protocol()
throw(std::runtime_error)
{
	memset(&client_addr,  0,  sizeof(client_addr)); 
	client_addr.sin_family 	    = AF_INET;  
	client_addr.sin_addr.s_addr = htons(INADDR_ANY);  
	client_addr.sin_port	    = htons(0);  

	return SUCCESS;
}


CBB_error CBB_tcp::
init_server_handle(ref_comm_handle_t  server_handle,
		   const std::string& my_uri,
	           int 		      port)
throw(std::runtime_error)
{

	struct sockaddr_in server_addr;
	memset(&server_addr,  0,  sizeof(server_addr)); 
	server_addr.sin_family 	    = AF_INET;  
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);  
	server_addr.sin_port	    = htons(port);  

	/*if (0 != my_uri.length())
	{
		if (0 == inet_aton(my_uri.c_str(), &server_addr.sin_addr))
		{
			perror("Server IP Address Error"); 
			throw std::runtime_error("Server IP Address Error");
		}
	}*/

	//create socket, throw rumtime error
	server_handle.socket=create_socket(server_addr);

	if(0 != listen(server_handle.socket,  MAX_QUEUE))
	{
		perror("Server Listen PORT ERROR");  
		throw std::runtime_error("Server Listen PORT ERROR");   
	}
	_DEBUG("start listening\n");

	return SUCCESS;
}

CBB_error CBB_tcp::
Connect(const char* uri,
	int 	    port,
	ref_comm_handle_t handle,
	void*	    buf,
	size_t*	    size)
throw(std::runtime_error)
{
	int count	  = 0; 
	struct sockaddr_in addr;

	addr.sin_family      = AF_INET;  
	addr.sin_addr.s_addr = htons(INADDR_ANY);  
	addr.sin_port 	     = htons(port);  

	if (0 == inet_aton(uri, &addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error");
	}

	//create socket, throw rumtime error
	handle.socket= create_socket(client_addr);

	while( MAX_CONNECT_TIME > ++count &&
			0 !=  connect(handle.socket, 
				reinterpret_cast<struct sockaddr*>(
					const_cast<struct sockaddr_in*>(&addr)),
				sizeof(addr)))
	{
		usleep(CONNECT_WAIT_TIME);
		_DEBUG("connect failed %d\n", count+1);
	}

	if (MAX_CONNECT_TIME == count)
	{
		close(handle.socket); 
		perror("Can not Connect to Server");  
		throw std::runtime_error("Can not Connect to Server"); 
	}
	_DEBUG("Connection established %d\n", handle.socket);

	//bad hack
	const char* my_uri=nullptr;
	get_uri_from_handle(&handle, &my_uri);
	push_back_uri(my_uri, buf, size);
	//init_message
	Do_send(&handle, buf, *size+MESSAGE_META_OFF,
			MSG_DONTWAIT);

	return SUCCESS; 
}

int CBB_tcp::
create_socket(const struct sockaddr_in& addr)
throw(std::runtime_error)
{
	int 		sockfd = 0;
	int 		on     = 1; 
	struct timeval  timeout;      
	timeout.tv_sec 	= 10;
	timeout.tv_usec = 0;

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

size_t CBB_tcp::
Do_recv(comm_handle_t   handle, 
	void* 		buffer,
	size_t 		size,
	int 		flag)
throw(std::runtime_error)
{
	int 	sockfd	  = handle->socket;
	size_t 	length 	  = size;
	ssize_t ret 	  = 0;
	char*	buff_tmp  = reinterpret_cast<char *>(buffer);

	if(0 == length)
	{
		return 0;
	}
	while(0 != length && 
			0 != (ret=recv(sockfd, buff_tmp, length, flag)))
	{
		if(-1 == ret)
		{
			if(EINTR == errno || EAGAIN == errno)
			{
				continue;
			}
			perror("do_recv");
			break;
		}
		buff_tmp += ret;
		length 	 -= ret;
	}
	if(0 == size-length)
	{
		_DEBUG("close socket %d\n", sockfd);
		close(sockfd);
		throw std::runtime_error("socket closed\n");
	}
	return size-length;
}

size_t CBB_tcp::
Do_send(comm_handle_t 	handle,
	const void* 	buffer, 
	size_t 		size, 
	int 		flag)
throw(std::runtime_error)
{
	int   	    sockfd   = handle->socket;
	ssize_t     ret      = 0;
	size_t	    length   = size; 
	const char* buff_tmp = reinterpret_cast<const char *>(
			buffer);

	if(0 == length)
	{
		return 0;
	}
	while(0 != length && 
			0 != (ret=send(sockfd, buff_tmp, length, flag)))
	{
		if(-1 == ret)
		{
			if(EINTR == errno || EAGAIN == errno)
			{
				continue;
			}
			perror("do_send");
			break;
		}
		buff_tmp += ret;
		length 	 -= ret;
	}
	if(0 == size - length)
	{
		_DEBUG("close socket %d\n", sockfd);
		close(sockfd);
		throw std::runtime_error("socket closed\n");
	}
	return size - length;
}

CBB_error CBB_tcp::
get_uri_from_handle(comm_handle_t handle,
		    const char**  uri)
{
	struct sockaddr_in addr;
	socklen_t len=sizeof(addr);
	getsockname(handle->socket, reinterpret_cast<struct sockaddr*>(&addr), &len);
	(*uri)=inet_ntoa(addr.sin_addr);
	return SUCCESS;
}
