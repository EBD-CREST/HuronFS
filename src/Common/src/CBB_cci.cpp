#include <pthread.h>

#include "comm_type/cci.h"

using namespace std;
using namespace CBB;
using namespace CBB::Common;

CBB_cci::CBB_cci():
	Comm_basic(),
	endpoint(nullptr),
{}

string CBB_cci::
get_uri(const char* 	server_addr,
	int 		server_port)
throw(std::runtime_error)
{
	int count	  = 0; 
	struct sockaddr_in addr;
	int socket	  = 0;
	size_t len	  = 0;
	char   uri_buf[URI_MAX]=0;

	addr.sin_family      = AF_INET;  
	addr.sin_addr.s_addr = htons(INADDR_ANY);  
	addr.sin_port 	     = htons(server_port);  

	if (0 == inet_aton(server_addr, &addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error");
	}

	//create socket, throw rumtime error
	socket= create_socket(client_addr);

	while( MAX_CONNECT_TIME > ++count &&
			0 !=  connect(handle->socket, 
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

	recv_by_tcp(socket, len, sizeof(len));
	recv_by_tcp(socket, uri_buf, len);
	close(socket);

	return string(uri_buf); 
}

size_t CBB_cci::
send_by_tcp(int     	  sockfd,
	    const char*   buf,
	    size_t  	  len)
{

	const char* buff_tmp=buf;
	size_t length=len;
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

	const char* buff_tmp=buf;
	size_t length=len;
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
connect_to_server(comm_handle_t handle)
	throw(std::runtime_error)
{
	comm_handle_t new_handle=
	struct timeval timeout;
	cci_conn_attribute_t attr=CCI_CONN_ATTR_RO;
	set_timer(&timeout);

	string server_uri=get_uri(server_addr, CCI_PORT);
	ret = cci_connect(endpoint, server_uri.c_str(), nullptr, 0, attr, nullptr,
			0, &timeout);
	return 
}

void* CBB_cci::
uri_exhange_thread_fun(void* args)
{
	struct epoll_event events[LENGTH_OF_LISTEN_QUEUE]; 
	struct sockaddr_in client_addr; 
	socklen_t length=sizeof(client_addr);
	const char* uri=reinterpret_cast<const char* args>(args);
	size_t uri_len=strlen(uri);
	
	memset(&events, 0, sizeof(events));
	event[0].data.fd=socket; 
	event[0].events=EPOLLIN; 
	epoll_ctl(epollfd, EPOLL_CTL_ADD, socket, &event); 

	while(true)
	{
		int nfds=epoll_wait(epollfd, events, LENGTH_OF_LISTEN_QUEUE, -1); 
		for(int i=0; i<nfds; ++i)
		{
			int socket=accept(server_socket,  reinterpret_cast<sockaddr *>(&client_addr),  &length);  
			if( 0 > socket)
			{
				fprintf(stderr,  "Server Accept Failed\n");  
				close(socket); 
				continue;
			}
			_DEBUG("A New Client\n"); 

			send_by_tcp(socket, uri_len, sizeof(uri_len));
			send_by_tcp(socket, uri, uri_len);
			close(socket);
		}
	}
	return nullptr;
}

int
init_server()
	throw(std::runtime_error)
{
	int ret;
	int caps;
	//cci_device_t * const * devices = nullptr;

	/*if (CCI_SUCCESS != 
			(ret = cci_get_devices(&devices))) 
	{
		_DEBUG("cci_get_devices() failed with %s\n",
				cci_strerror(nullptr, ret));
		return FAILURE;
	}*/

	if (CCI_SUCCESS != 
			(ret = cci_init(CCI_ABI_VERSION, 0, &caps))
	{
		_DEBUG("cci_init() failed with %s\n",
				cci_strerror(nullptr, ret));
		return FAILURE;
	}

	if (CCI_SUCCESS != 
			(ret = cci_create_endpoint(nullptr, 
					 0, &endpoint, nullptr)))
	{
		_DEBUG("cci_create_endpoint() failed with %s\n",
			cci_strerror(nullptr, ret));
		return FAILURE;
	}
	
	return SUCCESS;

}

CBB_error CBB_cci::
get_uri_from_handle(comm_handle_t handle,
		    char**	  uri)
{
	int ret;
	if (CCI_SUCCESS != 
			(ret = cci_get_opt(endpoint, CCI_OPT_ENDPT_URI, &uri)))
	{
		_DEBUG("cci_get_opt() failed with %s\n",
				cci_strerror(nullptr, ret));
		return FAILURE;
	}
	
	return SUCCESS;
}
