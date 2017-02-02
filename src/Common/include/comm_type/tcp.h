//library for tcp/ip communication


#ifndef CBB_TCP_H_
#define CBB_TCP_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>

#include "Comm_basic.h"

namespace CBB
{
	namespace Common
	{
		class CBB_tcp:
			public Comm_basic
		{
			public:
			CBB_tcp()=default;
		virtual	~CBB_tcp()=default;

		virtual size_t
			Do_recv(comm_handle_t 	handle, 
				void* 		buffer,
				size_t 		count,
				int 		flag)
			throw(std::runtime_error)override;
	
		virtual size_t 
			Do_send(comm_handle_t 	handle,
				const void* 	buffer, 
				size_t 		count, 
				int 		flag)
			throw(std::runtime_error)override;

		virtual size_t 
			Recv_large(comm_handle_t handle, 
				   void* 	 buffer,
				   size_t 	 count)
			throw(std::runtime_error)override;

		virtual size_t 
			Send_large(comm_handle_t handle, 
				   const void* 	 buffer,
				   size_t 	 count)
			throw(std::runtime_error)override;

		virtual CBB_error
			Close(comm_handle_t handle)override;

		virtual CBB_error
			Connect(const char*	uri,
				int		port,
				ref_comm_handle_t handle,
				void* 		buf,
				size_t*		size)
			throw(std::runtime_error)override; 

		virtual CBB_error 
			init_protocol()
			throw(std::runtime_error)override;

		virtual CBB_error 
			init_server_handle(ref_comm_handle_t  server_handle,
					   const std::string& my_uri,
				           int		      port)
			throw(std::runtime_error)override;

		virtual CBB_error
			end_protocol(comm_handle_t server_handle)override;
		
		virtual CBB_error
			get_uri_from_handle(comm_handle_t handle,
					    const char**  uri)override;

		virtual bool compare_handle(comm_handle_t src,
				comm_handle_t des)override;

		virtual const char* 
			get_my_uri()const;

			private:

		int 	create_socket(const struct sockaddr_in& addr)
			throw(std::runtime_error);

			private:
				struct sockaddr_in client_addr;
		};

		inline size_t CBB_tcp::
			Recv_large(comm_handle_t  handle, 
				   void* 	  buffer,
				   size_t 	  count)
			throw(std::runtime_error)
			{
				//simply forward
				return Do_recv(handle, buffer, count, MSG_DONTWAIT);
			}

		inline size_t CBB_tcp::
			Send_large(comm_handle_t  handle, 
				   const void* 	  buffer,
				   size_t 	  count)
			throw(std::runtime_error)
			{
				//simply forward
				return Do_send(handle, buffer, count, MSG_DONTWAIT);
			}

		inline int CBB_tcp::
			Close(comm_handle_t handle)
		{
			return close(handle->socket);
		}

		inline int CBB_tcp::
			end_protocol(comm_handle_t server_handle)
		{
			return close(server_handle->socket);
		}

		inline bool CBB_tcp::
			compare_handle(comm_handle_t src,
				comm_handle_t des)
		{
			return src->socket == des->socket;
		}

		inline const char* CBB_tcp::
			get_my_uri()const
		{
			return inet_ntoa(this->client_addr.sin_addr);
		}

	}
}
#endif
