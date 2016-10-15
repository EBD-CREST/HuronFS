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
#include <regex>

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
#ifdef CCI
			cci_connection_t*  	cci_handle; 
			cci_rma_handle_t*  	local_rma_handle; 
			cci_rma_handle  	remote_rma_handle; 
			const void*	 	buf;
			size_t		 	size;
#else
			int 		 	socket;
#endif
		};

		typedef CBB_handle comm_handle;
		typedef const CBB_handle* comm_handle_t;
		typedef CBB_handle* free_comm_handle_t;
		typedef CBB_handle& ref_comm_handle_t;

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
			
			virtual CBB_error init_protocol()
				throw(std::runtime_error)=0;

			virtual CBB_error 
				init_server_handle(ref_comm_handle_t server_handle,
					           int		      port)
				throw(std::runtime_error)=0;

			virtual CBB_error end_protocol(comm_handle_t server_handle)=0;

			virtual CBB_error Close(comm_handle_t handle)=0;

			virtual CBB_error get_uri_from_handle(comm_handle_t handle,
					const char**const  uri)=0;

			virtual bool compare_handle(comm_handle_t src,
					comm_handle_t des)=0;

			virtual CBB_error
				Connect(const char* uri,
					int	    port,
					ref_comm_handle_t handle,
					void* 	    buf,
					size_t*	    size)
				throw(std::runtime_error)=0;

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

			comm_handle_t 
				copy_handle(ref_comm_handle_t des,
						comm_handle_t src);

			void push_back_uri(const char* uri,
					   void*   buf,
					   size_t* size);
		protected:

			int create_socket(const struct sockaddr_in& addr)
				throw(std::runtime_error);

			void set_timer(struct timeval* timer);

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

		inline void Comm_basic::
			set_timer(struct timeval* timer)
			{
				timer->tv_sec=TIMEOUT_SEC;
				timer->tv_usec=0;
			}

		inline comm_handle_t 
			copy_handle(ref_comm_handle_t des,
				    comm_handle_t src)
			{
				memcpy(&des, src, sizeof(comm_handle));
				return &des;
			}

		inline bool
			is_ipaddr(const char* uri)
			{
				static std::regex regex_text("^([0-9]{1,3}\\.){3}[0-9]{1,3}$");
				std::string ip(uri);
				return std::regex_match(ip, regex_text); 
			}
		inline void Comm_basic::
			push_back_uri(	const char* uri,
					void*   buf,
				        size_t* size)
			{
				char* end_of_buf=reinterpret_cast<char*>(buf)+MESSAGE_META_OFF;
				size_t uri_len = strlen(uri)+1;
				memcpy(end_of_buf+(*size), &uri_len, sizeof(uri_len));
				*size+=sizeof(uri_len);
				memcpy(end_of_buf+(*size), uri, uri_len);
				*size+=uri_len;
			}

	}
}

#endif
