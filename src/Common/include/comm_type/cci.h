#ifndef CCI_H_
#define CCI_H_

#include "CBB_basic.h"

namespace CBB
{
	namespace Common
	{
		class CBB_cci():
			public Comm_basic
		{
			public:
			CBB_cci();
		virtual	~CBB_cci();

		virtual size_t
			Do_recv(comm_handle_t 	handle, 
				T* 		buffer,
				size_t 		count,
				int 		flag)
			throw(std::runtime_error)override;
	
		virtual size_t 
			Do_send(comm_handle_t 	handle,
				const T* 	buffer, 
				size_t 		count, 
				int 		flag)
			throw(std::runtime_error)override;

		virtual size_t 
			Recv_large(comm_handle_t handle, 
				   T* 		 buffer,
				   size_t 	 count)
			throw(std::runtime_error)override;

		virtual size_t 
			Send_large(comm_handle_t handle, 
				   const T* 	 buffer,
				   size_t 	 count)
			throw(std::runtime_error)override;

		virtual CBB_error
			get_uri_from_handle(comm_handle_t handle,
					    char**	  uri)override;

		virtual int 
			Close(comm_handle_t handle)override;

		comm_handle_t 
			connect_to_server(const char* server_addr,
					  int	      server_port)
			throw(std::runtime_error)override; 

		virtual int
			init_server()
			throw(std::runtime_error)override;

		virtual int
			stop_server()override;

		int 
			register_mem(const void*	ptr,
				     size_t 		size,
				     comm_handle_t	handle,
				     int		flag);
		static void*
			socket_thread_fun(void *obj);
		private:

		size_t send_by_tcp(int 	  	sockfd,
				   const char*  buf,
				   size_t  	len);

		size_t recv_by_tcp(int 	  	sockfd,
				   char*  	buf,
				   size_t  	len);
		private:
			cci_endpoint_t    endpoint;

		};

		inline int CBB_cci::
			Close(comm_handle_t 	  handle)
		{
			return cci_disconnect(handle->connection);
		}

		inline int CBB_cci::
			register_mem(const void*    ptr,
				     size_t 	    size,
				     comm_handle_t  handle,
				     int	    flag)
		{
			return cci_rma_register(handle->endpoint,
					ptr, size, flag, &handle->local_rma_handle);

		}
	}
}
#endif
