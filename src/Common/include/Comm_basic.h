/*
 * head file defines function used in tcp/ip communication
 */

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

		class basic_handle
		{
		public:
			basic_handle()=default;
			virtual ~basic_handle()=default;
			//define in each protocal
			basic_handle& operator = (const basic_handle& src);

			//API declearation
			template<class T> size_t 
				Recv(T& 	   buffer)
				throw(std::runtime_error);

			template<class T> size_t 
				Send(const T& 	   buffer)
				throw(std::runtime_error);

			template<class T> size_t 
				Recvv(T** buffer)
				throw(std::runtime_error);

			template<class T> size_t 
				Recvv_pre_alloc(T* 		buffer, 
						size_t 		length)
				throw(std::runtime_error);

			template<class T> size_t 
				Sendv(const T* 		buffer, 
				      size_t 		count)
				throw(std::runtime_error);

			template<class T> size_t 
				Sendv_pre_alloc(const T* 	buffer, 
						size_t 		count)
				throw(std::runtime_error);

			virtual CBB_error Close()=0;

			virtual CBB_error
				get_uri_from_handle(const char**  uri)=0;

			virtual bool compare_handle(const basic_handle* des)=0;

			virtual size_t 
				Recv_large(void*         buffer,
					   size_t 	 count)
				throw(std::runtime_error)=0;

			virtual size_t 
				Send_large(const void*   buffer,
					   size_t 	 count)
				throw(std::runtime_error)=0;

			virtual void*
				get_raw_handle()=0;

			virtual const void*
				get_raw_handle() const =0;

			virtual void
				set_raw_handle(void*)=0;

			virtual size_t
				do_recv(void* 	      buffer,
					size_t 	      count,
					int 	      flag)
				throw(std::runtime_error)=0;

			virtual size_t 
				do_send(const void* 	buffer, 
					size_t 		count, 
					int 		flag)
				throw(std::runtime_error)=0;
		};

		class Comm_basic
		{
			public:
				Comm_basic()=default;
				virtual ~Comm_basic()=default;

				virtual CBB_error 
					init_server_handle(basic_handle& server_handle,
							const std::string& my_uri,
							int		      port)
					throw(std::runtime_error)=0;

				virtual CBB_error
					Connect(const char* uri,
						int	    port,
						basic_handle& handle,
						void* 	    buf,
						size_t*	    size)
					throw(std::runtime_error)=0;

				virtual CBB_error init_protocol()
					throw(std::runtime_error)=0;

				virtual CBB_error end_protocol(const basic_handle* server_handle)=0;

				void push_back_uri(const char* uri,
						void*   buf,
						size_t* size);


			protected:

				int create_socket(const struct sockaddr_in& addr)
					throw(std::runtime_error);

				void set_timer(struct timeval* timer);

		};
		//implementation

		template<class T> inline size_t basic_handle::
			Recv(T& 	   buffer)
			throw(std::runtime_error)

			{
				return do_recv(&buffer, sizeof(T), MSG_WAITALL);
			}

		template<class T> inline size_t basic_handle::
			Send(const T& 	   buffer)
			throw(std::runtime_error)

			{
				return do_send(&buffer, sizeof(T), MSG_DONTWAIT|MSG_NOSIGNAL);
			}

		template<class T> inline size_t basic_handle::
			Recvv(T** 	    buffer)
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
					return do_recv(*buffer, length, MSG_WAITALL);
				}
			}

		template<class T> inline size_t basic_handle::
			Sendv(const T* 	    buffer,
			      size_t 	    count)
			throw(std::runtime_error)

			{
				//Send(sockfd, count);
				return do_send(buffer, count*sizeof(T),
						MSG_DONTWAIT|MSG_NOSIGNAL);
			}

		template<class T> inline size_t basic_handle::
			Recvv_pre_alloc(T* 	      buffer,
					size_t 	      count)
			throw(std::runtime_error)

			{
				return do_recv(buffer, count*sizeof(T), MSG_WAITALL);
			}

		template<class T> inline size_t basic_handle::
			Sendv_pre_alloc(const T*      buffer,
					size_t 	      count)
			throw(std::runtime_error)

			{
				return do_send(buffer, count*sizeof(T), MSG_DONTWAIT|MSG_NOSIGNAL);
			}

		inline void Comm_basic::
			set_timer(struct timeval* timer)
			{
				timer->tv_sec=TIMEOUT_SEC;
				timer->tv_usec=0;
			}

		inline bool
			is_ipaddr(const char* uri)
			{
				//should use this but disabled because of gcc issue
				static std::regex regex_text("^([0-9]{1,3}\\.){3}[0-9]{1,3}$");
				std::string ip(uri);
				return std::regex_match(ip, regex_text); 
				//return uri[0]<='9' && uri[0]>='0';
				
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
