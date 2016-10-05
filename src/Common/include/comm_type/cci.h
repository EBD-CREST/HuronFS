#ifndef CCI_H_
#define CCI_H_

#include "CBB_tcp.h"

namespace CBB
{
	namespace Common
	{
		class CBB_cci():
			public	CBB_tcp
		{
			public:
			CBB_cci();
		virtual	~CBB_cci();

		template<class T> virtual size_t
			Do_recv(comm_handle_t 	handle, 
				T* 		buffer,
				size_t 		count,
				int 		flag)
			throw(std::runtime_error)override;
	
		template<class T> virtual size_t 
			Do_send(comm_handle_t 	handle,
				const T* 	buffer, 
				size_t 		count, 
				int 		flag)
			throw(std::runtime_error)override;

		template<class T> virtual size_t 
			Recv_large(comm_handle_t handle, 
				   T* 		 buffer,
				   size_t 	 count)
			throw(std::runtime_error)override;

		template<class T> virtual size_t 
			Send_large(comm_handle_t handle, 
				   const T* 	 buffer,
				   size_t 	 count)
			throw(std::runtime_error)override;

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
			register_mem_server(const void*	ptr,
				     size_t 		size,
				     comm_handle_t	handle,
				     int		flag);
		int 
			register_mem_client(const void*	ptr,
				     size_t 		size,
				     comm_handle_t	handle,
				     int		flag);
		static void*
			socket_thread_fun(void *obj);

			private:
				cci_endpoint_t	server_endpoint;
				cci_endpoint_t	client_endpoint;
				char		completion_ptr;
				size_t		completion_length;
				char*		uri;
				
				pthread_t	socket_thread;//used to exchange uri
				int		epollfd;
				int		socket;
		};

		template<class T> size_t CBB_cci::
			Do_recv(comm_handle_t   handle, 
				T* 		buffer,
				size_t 		count,
				int 		flag)
			throw(std::runtime_error)
			{
				cci_handle_t cci_handle = handle->cci_handle;
				size_t	     length	= sizeof(T)*count;
				ssize_t	     ret 	= 0;
				char*	     buffer_tmp = reinterpret_cast<char *>(buffer);

				if(0 == length)
				{
					return 0;
				}

				if (nullptr == connection)
				{
					return 0;
				}

				return count*sizeof(T)-length;
			}

		template<class T> size_t CBB_cci::
			Do_send(comm_handle_t 	handle,
				const T* 	buffer, 
				size_t 		count, 
				int 		flag)
			throw(std::runtime_error)
			{
				cci_connection_t connection	= handle->connection;
				size_t 		 length		= sizeof(T)*count;
				int		 ret 		= 0;

				if (nullptr == connection)
				{
					return 0;
				}

				if(0 == length)
				{
					return 0;
				}

				ret = cci_send(connection,
					       buffer,
					       length,
					       handle,
					       flag);

				if (CCI_SUCCESS != ret)
				{
					_DEBUG("error %s\n",
							cci_strerror
							(endpoint, ret));
				}
				else
				{

					Close(handle);
					throw std::runtime_error("connection closed\n");
				}
				return length;
			}

		template<class T> inline size_t CBB_cci::
			Recv_large(comm_handle_t handle, 
				   T* 		 buffer,
				   size_t 	 count)
			throw(std::runtime_error)
			{
				cci_connection_t connection	= handle->connection;
				size_t 		 length		= sizeof(T)*count;
				int		 ret 		= 0;

				if (nullptr == connection)
				{
					return 0;
				}

				if(0 == length)
				{
					return 0;
				}

				ret = cci_rma(connection,
					      buffer, length,
					      connection->local_rma_handle, 0,
					      connection->remote_rma_handle, 0,
					      length, handle,
					      CCI_FLAG_READ);

				if (CCI_SUCCESS != ret)
				{
					_DEBUG("error %s\n",
							cci_strerror
							(endpoint, ret));
				}
				else
				{
					Close(handle);
					throw std::runtime_error("connection closed\n");
				}
				return length;

			}

		template<class T> inline size_t CBB_cci::
			Send_large(comm_handle_t  handle, 
				   const T* 	  buffer,
				   size_t 	  count)
			throw(std::runtime_error)
			{
				cci_connection_t connection	= handle->connection;
				size_t 		 length		= sizeof(T)*count;
				int		 ret 		= 0;

				if (nullptr == connection)
				{
					return 0;
				}

				if(0 == length)
				{
					return 0;
				}

				ret = cci_rma(connection,
					      buffer, length,
					      connection->local_rma_handle, 0,
					      connection->remote_rma_handle, 0,
					      length, handle,
					      CCI_FLAG_WRITE);

				if (CCI_SUCCESS != ret)
				{
					_DEBUG("write error %s\n",
							cci_strerror
							(endpoint, ret));
				}
				else
				{
					Close(handle);
					throw std::runtime_error(
							"connection closed\n");
				}
				return length;

			}

		inline int CBB_cci::
			Close(comm_handle_t 	  handle)
		{
			return cci_disconnect(handle->connection);
		}

		inline int CBB_cci::
			register_mem_server(const void*	   ptr,
				            size_t 	   size,
				     	    comm_handle_t  handle,
				     	    int		   flag)
		{
			return cci_rma_register(server_endpoint,
					ptr, size, flag, &handle);

		}

		inline int CBB_cci::
			register_mem_client(const void*	  ptr,
				            size_t 	  size,
				     	    comm_handle_t handle,
				     	    int		  flag)
		{
			return cci_rma_register(client_endpoint,
					ptr, size, flag, &handle);

		}
	}
}
#endif
