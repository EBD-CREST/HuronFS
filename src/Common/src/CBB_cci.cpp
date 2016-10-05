#include <pthread.h>

#include "comm_type/cci.h"

using namespace std;
using namespace CBB;
using namespace CBB::Common;

CBB_cci::CBB_cci():
	CBB_tcp(),
	server_endpoint(),
	client_endpoint(),
	completion_ptr("D"),
	completion_length(sizeof(char)),
	uri(nullptr)
{}


comm_handle_t CBB_cci::
connect_to_server(const char* server_addr,
		  int	      server_port)
	throw(std::runtime_error)
{
	if(-1 == (epollfd=epoll_create(LENGTH_OF_LISTEN_QUEUE+1)))
	{
		perror("epoll_creation"); 
		throw std::runtime_error("epllfd error\n");
	}
	//wait on socket
	pthread_create(socket_thread, nullptr,
		       socket_thread_fun, this); 
}

void* CBB_cci::
socket_thread_fun(void* obj)
{
	CBB_cci* this_obj=reinterpret_cast<CBB_cci*>(obj);
	struct epoll_event events[LENGTH_OF_LISTEN_QUEUE]; 
	
	memset(&events, 0, sizeof(events));
	event[0].data.fd=socket; 
	event[0].events=EPOLLIN; 
	epoll_ctl(this_obj->epollfd, EPOLL_CTL_ADD, this_obj->socket, &event); 

	while(true)
	{
		int nfds=epoll_wait(this_obj->epollfd, events, LENGTH_OF_LISTEN_QUEUE, -1); 
		for(int i=0; i<nfds; ++i)
		{
			int new_socket=accept(
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

	if (CCI_SUCCESS != 
			(ret = cci_get_opt(endpoint, CCI_OPT_ENDPT_URI, &uri)))
	{
		_DEBUG("cci_get_opt() failed with %s\n",
				cci_strerror(nullptr, ret));
		return FAILURE;
	}
	
	return SUCCESS;
}
