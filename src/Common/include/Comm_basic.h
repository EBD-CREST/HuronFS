/*
 * head file defines function used in tcp/ip communication
 */

#ifndef COMM_BASIC_H_
#define COMM_BASIC_H_

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <exception>

#include "CBB_internal.h"
#include "CBB_const.h"
#include "CBB_error.h"

#ifdef CCI
#include "cci.h"
#endif

namespace CBB
{
	namespace Common
	{

		//protocal name 
		const int PROT_TCP=1;
		const int PROT_CCI=2;
		struct CBB_handle
		{
			int 		 protocol_name;
#ifdef CCI
			cci_connection_t cci_handle; 
			cci_rma_handle_t local_rma_handle; 
			cci_rma_handle_t remote_rma_handle; 
#else
			int 		 socket;
			int		 port;
#endif
		};

		typedef CBB_handle comm_handle;
		typedef CBB_handle* comm_handle_t;
		typedef const CBB_handle* const_comm_handle_t;

		class Comm_basic
		{

		public:
			Comm_basic()=default;
			virtual ~Comm_basic()=default;
			Comm_basic(const Comm_basic&)=delete;
			Comm_basic& operator = (const Comm_basic&)=delete;

			//API declearation
			template<class T> size_t 
				Recv(comm_handle_t handle, 
				     T& 	   buffer)
				throw(std::runtime_error);

			template<class T> size_t 
				Send(comm_handle_t handle, 
				     const T& 	   buffer)
				throw(std::runtime_error);

			template<class T> size_t 
				Recvv(comm_handle_t handle, 
				      T** buffer)
				throw(std::runtime_error);

			template<class T> size_t 
				Recvv_pre_alloc(comm_handle_t 	handle, 
						T* 		buffer, 
						size_t 		length)
				throw(std::runtime_error);

			template<class T> size_t 
				Sendv(comm_handle_t 	handle, 
				      const T* 		buffer, 
				      size_t 		count)
				throw(std::runtime_error);

			template<class T> size_t 
				Sendv_pre_alloc(comm_handle_t 	handle, 
						const T* 	buffer, 
						size_t 		count)
				throw(std::runtime_error);
			
			virtual comm_handle_t create_handle()=0;

			virtual CBB_error init_protocol()
				throw(std::runtime_error)=0;

			virtual CBB_error 
				init_server(comm_handle_t server_handle)
				throw(std::runtime_error)=0;

			virtual CBB_error end_protocol(comm_handle_t server_handle)=0;

			virtual CBB_error Close(comm_handle_t handle)=0;

			virtual CBB_error get_uri_from_handle(comm_handle_t handle,
					char** uri)=0;

			virtual bool compare_handle(comm_handle_t src,
					comm_handle_t des)=0;

			virtual comm_handle_t copy_handle(comm_handle_t src,
					comm_handle_t des)=0;

			virtual size_t 
				Recv_large(comm_handle_t sockfd, 
					   void*         buffer,
					   size_t 	 count)
				throw(std::runtime_error)=0;

			virtual size_t 
				Send_large(comm_handle_t sockfd, 
					   const void*   buffer,
					   size_t 	 count)
				throw(std::runtime_error)=0;

		private:
			virtual size_t
				Do_recv(comm_handle_t sockfd, 
					void* 	      buffer,
					size_t 	      count,
					int 	      flag)
				throw(std::runtime_error)=0;

			virtual size_t 
				Do_send(comm_handle_t 	sockfd,
					const void* 	buffer, 
					size_t 		count, 
					int 		flag)
				throw(std::runtime_error)=0;

		};

		//implementation

		template<class T> inline size_t Comm_basic::
			Recv(comm_handle_t handle,
			     T& 	   buffer)
			throw(std::runtime_error)

			{
				return Do_recv(handle, &buffer, sizeof(T), MSG_WAITALL);
			}

		template<class T> inline size_t Comm_basic::
			Send(comm_handle_t handle,
			     const T& 	   buffer)
			throw(std::runtime_error)

			{
				return Do_send(handle, &buffer, sizeof(T), MSG_DONTWAIT|MSG_NOSIGNAL);
			}

		template<class T> inline size_t Comm_basic::
			Recvv(comm_handle_t handle,
			      T** 	    buffer)
			throw(std::runtime_error)

			{
				size_t length;
				//Recv(sockfd, length);
				if(NULL == (*buffer=new char[length+1]))
				{
					perror("new");
					return 0;
				}
				else
				{
					(*buffer)[length]=0;
					return Do_recv(handle, *buffer, length, MSG_WAITALL);
				}
			}

		template<class T> inline size_t Comm_basic::
			Sendv(comm_handle_t handle,
			      const T* 	    buffer,
			      size_t 	    count)
			throw(std::runtime_error)

			{
				//Send(sockfd, count);
				return Do_send(handle, buffer,
						  count*sizeof(T), MSG_DONTWAIT|MSG_NOSIGNAL);
			}

		template<class T> inline size_t Comm_basic::
			Recvv_pre_alloc(comm_handle_t handle,
					T* 	      buffer,
					size_t 	      count)
			throw(std::runtime_error)

			{
				return Do_recv(handle, buffer,
						  count*sizeof(T), MSG_WAITALL);
			}

		template<class T> inline size_t Comm_basic::
			Sendv_pre_alloc(comm_handle_t handle,
					const T*      buffer,
					size_t 	      count)
			throw(std::runtime_error)

			{
				return Do_send(handle, buffer,
						  count*sizeof(T), MSG_DONTWAIT|MSG_NOSIGNAL);
			}
	}
}

#endif
