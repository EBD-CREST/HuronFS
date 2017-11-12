/*
 * Copyright (c) 2017, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. 
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp, LLNL-CODE-722817.
 * All rights reserved. 
 *
 * This file is part of HuronFS.
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

// library for tcp/ip communication

#ifndef CBB_TCP_H_
#define CBB_TCP_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>

#include "Comm_basic.h"

namespace CBB
{
	namespace Common
	{
		class CBB_tcp;

		class TCP_handle:
			public basic_handle
		{
		public:
			TCP_handle()=default;
			virtual ~TCP_handle()=default;
			friend CBB_tcp;

			virtual size_t
				do_recv(void* 		buffer,
					size_t 		count,
					int 		flag)
				throw(std::runtime_error)override final;

			virtual size_t 
				do_send(const void* 	buffer, 
					size_t 		count, 
					int 		flag)
				throw(std::runtime_error)override final;

			virtual size_t 
				Recv_large(void* 	 buffer,
					   size_t 	 count)
				throw(std::runtime_error)override final;

			virtual size_t 
				Send_large(const void* 	 buffer,
					   size_t 	 count)
				throw(std::runtime_error)override final;

			virtual CBB_error
				Close()override final;

			virtual CBB_error
				get_uri_from_handle(const char**  uri)override final;

			virtual bool compare_handle(const basic_handle* des)override final;

			virtual void*
				get_raw_handle()override final;

			virtual const void*
				get_raw_handle()const override final;

			virtual void
				set_raw_handle(void*)override final;

			TCP_handle& operator = (const TCP_handle& src);

		private:
			int 		 	socket;
		};

		class CBB_tcp:
			public Comm_basic
		{
			public:
			CBB_tcp()=default;
		virtual	~CBB_tcp()=default;


		virtual CBB_error
			Connect(const char*	uri,
				int		port,
				basic_handle& handle,
				void* 		buf,
				size_t*		size)
			throw(std::runtime_error)override; 

		virtual CBB_error 
			init_protocol()
			throw(std::runtime_error)override;

		virtual CBB_error 
			init_server_handle(basic_handle& server_handle,
					   const std::string& my_uri,
				           int		      port)
			throw(std::runtime_error)override;

		virtual CBB_error
			end_protocol(const basic_handle* server_handle)override;
		

		virtual const char* 
			get_my_uri()const;

			private:

		int 	create_socket(const struct sockaddr_in& addr)
			throw(std::runtime_error);

			private:
				struct sockaddr_in client_addr;
		};

		inline size_t TCP_handle::
			Recv_large(void* 	  buffer,
				   size_t 	  count)
			throw(std::runtime_error)
			{
				//simply forward
				return do_recv(buffer, count, MSG_WAITALL);
			}

		inline size_t TCP_handle::
			Send_large(const void* 	  buffer,
				   size_t 	  count)
			throw(std::runtime_error)
			{
				//simply forward
				return do_send(buffer, count, MSG_DONTWAIT);
			}

		inline int TCP_handle::
			Close()
		{
			return close(this->socket);
		}

		inline bool TCP_handle::
			compare_handle(const basic_handle* des)
		{
			const int* des_socket=static_cast<const int*>(des->get_raw_handle());
			return this->socket == *des_socket;
		}

		inline int CBB_tcp::
			end_protocol(const basic_handle* server_handle)
		{
			const int* server_socket=static_cast<const int*>(server_handle->get_raw_handle());
			return close(*server_socket);
		}

		inline const char* CBB_tcp::
			get_my_uri()const
		{
			return inet_ntoa(this->client_addr.sin_addr);
		}

		inline TCP_handle& TCP_handle::
			 operator = (const TCP_handle& src)
		{
			memcpy(this, &src, sizeof(src));
			return *this;
		}

		inline void* 
			TCP_handle::get_raw_handle()
		{
			return &this->socket;
		}

		inline const void* 
			TCP_handle::get_raw_handle() const
		{
			return &this->socket;
		}

		inline void
			TCP_handle::set_raw_handle(void* handle)
		{
			int* socket=static_cast<int*>(handle);
			this->socket=*socket;
		}
	}
}
#endif
