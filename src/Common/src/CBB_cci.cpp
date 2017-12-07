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

#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "comm_type/CBB_cci.h"

using namespace std;
using namespace CBB;
using namespace CBB::Common;

CBB_error CBB_cci::
get_uri_from_server(const char* server_addr,
		    int 	server_port,
		    char*	uri_buf)
throw(std::runtime_error)
{
	int count	  = 0; 
	struct sockaddr_in addr;
	int socket	  = 0;
	size_t len	  = 0;
	struct sockaddr_in client_addr;

	addr.sin_family      = AF_INET;  
	addr.sin_addr.s_addr = htons(INADDR_ANY);  
	addr.sin_port 	     = htons(server_port);  

	client_addr.sin_family      = AF_INET;  
	client_addr.sin_addr.s_addr = htons(INADDR_ANY);  
	client_addr.sin_port 	    = htons(0);  

	if (0 == inet_aton(server_addr, &addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		_DEBUG("Server addr %s port %d\n", server_addr, server_port);
		throw std::runtime_error("Server IP Address Error");
	}

	//create socket, throw rumtime error
	socket= create_socket(client_addr);

	while( MAX_CONNECT_TIME > ++count &&
			0 !=  connect(socket, 
				reinterpret_cast<struct sockaddr*>(
					const_cast<struct sockaddr_in*>(&addr)),
				sizeof(addr)))
	{
		usleep(CONNECT_WAIT_TIME);
		_DEBUG("connect failed %d\n", count+1);
	}

	if (MAX_CONNECT_TIME == count)
	{
		close(socket); 
		perror("Can not Connect to Server");  
		throw std::runtime_error("Can not Connect to Server"); 
	}

	recv_by_tcp(socket, reinterpret_cast<char*>(&len), sizeof(len));
	recv_by_tcp(socket, uri_buf, len);
	uri_buf[len]='\0';
	close(socket);

	return SUCCESS;
}

handle_context::
handle_context(int id, handle_context* next):
	basic_task(id, next),
	local_rma_handle(nullptr),
	block_ptr(nullptr)
{}

handle_context::
handle_context():
	basic_task(),
	local_rma_handle(nullptr),
	block_ptr(nullptr)
{}

size_t CBB_cci::
send_by_tcp(int     	  sockfd,
	    const char*   buf,
	    size_t  	  len)
{

	const char* buff_tmp=buf;
	size_t length=len;
	ssize_t ret;

	while(0 != length && 
			0 != (ret=send(sockfd, buff_tmp, length, MSG_DONTWAIT)))
	{
		if(-1 == ret)
		{
			if(EINTR == errno || EAGAIN == errno)
			{
				continue;
			}
			perror("send_by_tcp");
			break;
		}
		buff_tmp += ret;
		length 	 -= ret;
	}

	return len-length;
}

size_t CBB_cci::
recv_by_tcp(int      sockfd,
	    char*    buf,
	    size_t   len)
{

	char* buff_tmp=buf;
	size_t length=len;
	ssize_t ret=0;

	while(0 != length && 
			0 != (ret=recv(sockfd, buff_tmp, length, MSG_WAITALL)))
	{
		if(-1 == ret)
		{
			if(EINTR == errno || EAGAIN == errno)
			{
				continue;
			}
			perror("recv_by_tcp");
			break;
		}
		buff_tmp += ret;
		length 	 -= ret;
	}

	return len-length;
}

CBB_error CBB_cci::
start_uri_exchange_server(const std::string& 	my_uri,
		          int 			port)
throw(std::runtime_error)
{
	struct sockaddr_in server_addr;
	struct epoll_event event; 
	int ret;

	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family      = AF_INET;  
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);  
	server_addr.sin_port 	    = htons(port);  
	
	this->args.socket=create_socket(server_addr);
	if(0 != listen(this->args.socket,  MAX_QUEUE))
	{
		perror("Server Listen PORT ERROR");  
		throw std::runtime_error("Server Listen PORT ERROR");   
	}
	if(-1 == (this->args.epollfd=epoll_create(LENGTH_OF_LISTEN_QUEUE+1)))
	{
		perror("epoll_creation"); 
		throw std::runtime_error("CBB_communication_thread"); 
	}

	memset(&event, 0, sizeof(event));
	event.data.fd=this->args.socket;
	event.events=EPOLLIN; 
		
	if( -1 == epoll_ctl(this->args.epollfd, EPOLL_CTL_ADD, this->args.socket, &event)) 
	{
		perror("Server Listen PORT ERROR");  
		throw std::runtime_error("Server Listen PORT ERROR");   
	}

	this->args.uri=uri;
	this->args.uri_len=strlen(uri);

	if(0 != 
		(ret=pthread_create(&(this->exchange_thread), 
				    nullptr, uri_exchange_thread_fun, &args)))
	{
		throw std::runtime_error("create_thread");
	}

	return SUCCESS;
}

void* CBB_cci::
uri_exchange_thread_fun(void* args)
{
	struct epoll_event events[LENGTH_OF_LISTEN_QUEUE]; 
	struct sockaddr_in client_addr;
	socklen_t length=sizeof(client_addr);
	struct exchange_args* args_ptr=reinterpret_cast<struct exchange_args*>(args);
	const char* uri=args_ptr->uri;
	int epollfd=args_ptr->epollfd;
	int server_socket=args_ptr->socket;
	size_t uri_len=strlen(uri);
	_DEBUG("start uri exchange thread\n");
	
	while(true)
	{
		int nfds=epoll_wait(epollfd, events, LENGTH_OF_LISTEN_QUEUE, -1); 
		for(int i=0; i<nfds; ++i)
		{
			int socket=accept(server_socket, 
					reinterpret_cast<sockaddr *>(&client_addr), 
					&length);  
			if( 0 > socket)
			{
				fprintf(stderr,  "Server Accept Failed\n");  
				close(socket); 
				continue;
			}
			_DEBUG("A New Client\n"); 

			send_by_tcp(socket, 
					reinterpret_cast<char*>(&uri_len), sizeof(uri_len));
			send_by_tcp(socket, uri, uri_len);
			close(socket);
		}
	}
	_DEBUG("end thread\n");
	return nullptr;
}

int CBB_cci::
init_protocol()
	throw(std::runtime_error)
{
	int   ret	= 0;
	uint32_t caps	= 0;

	//cci_device_t * const * devices = nullptr;

	/*if (CCI_SUCCESS != 
			(ret = cci_get_devices(&devices))) 
	{
		_DEBUG("cci_get_devices() failed with %s\n",
				cci_strerror(nullptr, ret));
		return FAILURE;
	}*/

	if (CCI_SUCCESS != 
		(ret = cci_init(CCI_ABI_VERSION, 0, &caps)))
	{
		_DEBUG("cci_init() failed with %s\n",
				cci_strerror(nullptr, (cci_status)ret));
		return FAILURE;
	}

	if (CCI_SUCCESS != 
		(ret = cci_create_endpoint(nullptr, 
					 0, &(this->endpoint), nullptr)))
	{
		_DEBUG("cci_create_endpoint() failed with %s\n",
			cci_strerror(nullptr, (cci_status)ret));
		return FAILURE;
	}

	if (CCI_SUCCESS !=
		(ret = cci_get_opt(this->endpoint,
			  CCI_OPT_ENDPT_URI, &(this->uri))))
	{
		_DEBUG("cci_get_opt() failed with %s\n",
			cci_strerror(nullptr, (cci_status)ret));
		return FAILURE;
	}
	
	return SUCCESS;

}

CBB_error CBB_cci::
init_server_handle(basic_handle& server_handle,
		   const std::string& my_uri,
		   int		      port)
throw(std::runtime_error)
{
	CCI_handle& server_cci_handle=dynamic_cast<CCI_handle&>(server_handle);
	server_cci_handle.setup_uri(this->uri);
	return start_uri_exchange_server(my_uri, port);
}


CBB_error CBB_cci::
end_protocol(const basic_handle* server_handle)
{

	int ret;
	if (CCI_SUCCESS != 
		(ret = cci_finalize())) 
	{
		_DEBUG("cci_finalize() failed with %s\n",
			cci_strerror(nullptr, (cci_status)ret));
		return FAILURE;
	}
	else
	{
		return SUCCESS;
	}
}

CBB_error CCI_handle::
Close()
{

	int ret;

	if (CCI_SUCCESS != 
		(ret = cci_disconnect(this->cci_handle))) 
	{
		_DEBUG("cci_disconnect() failed with %s\n",
			cci_strerror(nullptr, (cci_status)ret));
		return FAILURE;
	}
	_DEBUG("cci_handle failed %p\n", cci_handle);
	return SUCCESS;
}

CBB_error CCI_handle::
get_uri_from_handle(const char **const	uri)
{
	*uri=this->uri;
	return SUCCESS;
}

CBB_error CBB_cci::
Connect(const char* uri,
	int	    port,
	basic_handle& handle,
	void* 	    buf,
	size_t*	    size)
	throw(std::runtime_error)
{
	char server_uri[URI_MAX];
	int ret;
	if(is_ipaddr(uri))
	{
		get_uri_from_server(uri, port, server_uri);
	}
	else
	{
		memcpy(server_uri, uri, strlen(uri));
		server_uri[strlen(uri)]='\0';
	}

	struct timeval timeout;
	cci_conn_attribute_t attr=CCI_CONN_ATTR_RO;
	set_timer(&timeout);

	push_back_uri(this->uri, buf, size);
	if(CCI_SUCCESS != 
			(ret = cci_connect(endpoint, server_uri, 
					   buf, *size+MESSAGE_META_OFF,
					   attr, &handle, 0, &timeout)))
	{
		printf("cci_connect() failed with %s\n",
			cci_strerror(nullptr, (cci_status)ret));
		return FAILURE;
	}
	return SUCCESS;
}

size_t CCI_handle:: 
Recv_large(const unsigned char*   completion,
	   size_t 	 comp_size,
	   void*         buffer,
	   size_t 	 count,
	   off64_t	 offset,
	   void*	 context)
throw(std::runtime_error)
{
	int ret;
	cci_connection_t* connection=cci_handle;
	//first try with blocking IO
	_DEBUG("rma read local handle=%p\n", local_rma_handle);
	if(CCI_SUCCESS != 
		(ret = cci_rma(connection, completion, comp_size,
			       local_rma_handle, 0,
			       &remote_rma_handle, offset,
			       count, context, CCI_FLAG_READ)))
	{
		if(CCI_ERR_DISCONNECTED == ret)
		{
			_DEBUG("connection close\n");
			throw std::runtime_error("connection closed");
		}
		_DEBUG("cci_rma() failed with %s\n",
			cci_strerror(nullptr, (cci_status)ret));
		return FAILURE;
	}
	return SUCCESS;
}

size_t CCI_handle::
Send_large(const unsigned char*   completion,
	   size_t 	 comp_size,
	   const void*   buffer,
	   size_t 	 count,
	   off64_t	 offset,
	   void*	 context)
throw(std::runtime_error)
{
	int ret;
	cci_connection_t* connection=cci_handle;

	//first try with blocking IO
	_DEBUG("rma write local handle=%p\n", local_rma_handle);
	if(CCI_SUCCESS != 
		(ret = cci_rma(connection, completion, comp_size,
			       local_rma_handle, 0,
			       &remote_rma_handle, offset,
			       count, context, CCI_FLAG_WRITE)))
	{
		if(CCI_ERR_DISCONNECTED == ret)
		{
			_DEBUG("connection close\n");
			throw std::runtime_error("connection closed");
		}
		_DEBUG("cci_rma() failed with %s\n",
			cci_strerror(nullptr, (cci_status)ret));
		return FAILURE;
	}
	return SUCCESS;
}

size_t CCI_handle::
do_recv(void* 	      buffer,
	size_t 	      count,
	int 	      flag)
throw(std::runtime_error)
{
	if(count > this->size)
	{
		count=this->size;
	}
	memcpy(buffer, this->buf, count);
	//bad hack
	const void**	buf_tmp=(const void**)(&this->buf);
	*buf_tmp=(const void*)(static_cast<const char*const>(this->buf)+count);
	return count;
}

size_t CCI_handle::
do_send(const void* 	buffer, 
	size_t 		count, 
	int 		flag)
throw(std::runtime_error)
{
	int ret;
	size_t ans=count;
	_DEBUG("send data to %p\n", cci_handle);
	while(0 != count)
	{
		if(CCI_SUCCESS != 
			(ret=cci_send(this->cci_handle,
				      buffer, count,
				      nullptr, CCI_FLAG_BLOCKING)))
		{
			if(CCI_ERR_DISCONNECTED == ret)
			{
				_DEBUG("connection close\n");
				throw std::runtime_error("connection closed");
			}
			_DEBUG("cci_send error %s\n",
					cci_strerror(nullptr, (cci_status)ret));
		}
		else
		{
			count-=count;
		}
	}
	return ans;
}

CBB_error CBB_cci::
register_mem(void*	    ptr,
		size_t 	    size,
		CCI_handle*    handle,
		int	    flag)
{
	int ret;
	_DEBUG("register mem %p size=%ld\n",
			ptr, size);
	if (CCI_SUCCESS != (ret=cci_rma_register(endpoint,
					ptr, size, flag, 
					const_cast<cci_rma_handle_t**>(&(handle->local_rma_handle)))))
	{
		_DEBUG("cci_register() failed with %s\n",
				cci_strerror(endpoint, (cci_status)ret));
		return FAILURE;
	}
	handle->dump_local_key();
	return SUCCESS;
}

CBB_error CBB_cci::
deregister_mem(CCI_handle* handle)
{
	int ret;

	_DEBUG("deregister mem\n");
	if (CCI_SUCCESS != (ret=cci_rma_deregister(endpoint,
					handle->local_rma_handle)))
	{
		_DEBUG("cci_deregister() failed with %s\n",
				cci_strerror(endpoint, (cci_status)ret));
		return FAILURE;
	}
	return SUCCESS;
}

handle_context* CBB_cci::
allocate_communication_context(CCI_handle* handle, void* context)
{
	handle_context* context_ptr=context_queue.allocate_tmp_node();
	context_ptr->local_rma_handle=handle->local_rma_handle;
	context_ptr->block_ptr=context;
	context_queue.task_enqueue_no_notification();
	
	return context_ptr;
}

void CBB_cci::
putback_communication_context()
{
	context_queue.task_dequeue();
	return;
}
