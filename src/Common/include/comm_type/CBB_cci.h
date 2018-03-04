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

#ifndef CCI_H_
#define CCI_H_

#include "Comm_basic.h"
#include "CBB_task_parallel.h"

namespace CBB
{
	namespace Common
	{
		struct exchange_args
		{
			int epollfd;
			int socket;
			const char* uri;
			size_t uri_len;
		};

		class CBB_cci;

		class handle_context:public basic_task
		{
		public:
			handle_context();
			handle_context(int id, handle_context* next);
			virtual ~handle_context()=default;
			virtual void reset()override final;

			cci_rma_handle_t*  	local_rma_handle; 
			void*			block_ptr;
			int			mode;
		};

		typedef task_parallel_queue<handle_context> context_queue_t;

		class CCI_handle:
			public basic_handle
		{
		public:
			CCI_handle()=default;
			virtual ~CCI_handle()=default;
			friend class CBB_cci;

			virtual CBB_error Close()override final;

			virtual bool compare_handle(const basic_handle* des)override final;

			virtual CBB_error
				get_uri_from_handle(const char**      uri)override final;

			virtual size_t 
				Recv_large(void*         buffer,
					   size_t 	 count,
					   void*	 context)
				throw(std::runtime_error)override final;

			virtual size_t 
				Send_large(const void*   buffer,
					   size_t 	 count,
					   void*	 context)
				throw(std::runtime_error)override final;

			size_t Recv_large(void*         buffer,
					  size_t 	count,
					  off64_t	offset,
					  void*		context)
				throw(std::runtime_error);

			size_t Send_large(const void*   buffer,
					  size_t 	count,
					  off64_t	offset,
					  void*		context)
				throw(std::runtime_error);

			size_t Recv_large(const unsigned char*   completion,
					  size_t	comp_size,
					  void*         buffer,
					  size_t 	count,
					  off64_t	offset,
					  void*		context)
				throw(std::runtime_error);

			size_t Send_large(const unsigned char*   completion,
					  size_t	comp_size,
					  const void*   buffer,
					  size_t 	count,
					  off64_t	offset,
					  void*		context)
				throw(std::runtime_error);
			virtual size_t
				do_recv(void* 	      buffer,
					size_t 	      count,
					int 	      flag)
				throw(std::runtime_error)override final;

			virtual size_t 
				do_send(const void* 	buffer, 
					size_t 		count, 
					int 		flag)
				throw(std::runtime_error)override final;

			CCI_handle& operator = (const CCI_handle& src);
			void dump_remote_key()const;
			void dump_local_key() const;

			virtual void*
				get_raw_handle()override final;

			virtual const void*
				get_raw_handle() const override final;

			virtual void
				set_raw_handle(void*)override final;
			void setup_uri(const char* uri);
		public:
			cci_connection_t*  	cci_handle; 
			cci_rma_handle_t*  	local_rma_handle; 
			cci_rma_handle  	remote_rma_handle; 
			//use for the uri
			const void*	 	buf;
			size_t		 	size;
			const char*		uri;

		};

		class CBB_cci:
			public Comm_basic
		{
			public:
			CBB_cci()=default;
		virtual	~CBB_cci()=default;

		virtual CBB_error init_protocol()
			throw(std::runtime_error)override final;

		virtual CBB_error 
			init_server_handle(basic_handle&      server_handle,
					   const std::string& my_uri,
					   int		      port)
			throw(std::runtime_error)override final;

		virtual CBB_error end_protocol(const basic_handle*  server_handle)override final;

		virtual CBB_error
			Connect(const char* uri,
				int	    port,
				basic_handle& handle,
				void* 	    buf,
				size_t*	    size)
			throw(std::runtime_error)override final;


		CBB_error start_uri_exchange_server(const std::string& 	my_uri,
						    int 		port)
			throw(std::runtime_error);

		static void*
			socket_thread_fun(void *obj);
		
		cci_endpoint_t* get_endpoint();

		virtual const char*
			get_my_uri()const;
		void dump_remote_key()const;

		CBB_error register_mem(
				void*		ptr,
				size_t 		size,
				CCI_handle*  	handle,
				int		flag);

		CBB_error deregister_mem(CCI_handle* handle);
		CBB_error deregister_mem(cci_rma_handle_t* handle);

		handle_context* allocate_communication_context(CCI_handle* handle, void* block, int mode);
		handle_context* putback_communication_context();

		private:

		static size_t 
			send_by_tcp(int   	sockfd,
				    const char* buf,
				    size_t  	len);

		static size_t 
			recv_by_tcp(int   	sockfd,
				    char*  	buf,
				    size_t  	len);

		CBB_error
			get_uri_from_server(const char* server_addr,
					    int 	server_port,
					    char*	uri_buf)
			throw(std::runtime_error);

		static void* 
			uri_exchange_thread_fun(void* args);

		protected:
			cci_endpoint_t*   endpoint;
			const char*	  uri;
			pthread_t	  exchange_thread;
			struct exchange_args args;

			context_queue_t context_queue;

		};

		inline bool CCI_handle::
			compare_handle(const basic_handle* des)
			{
				auto des_handle=static_cast<const cci_connection_t*>(des->get_raw_handle());
				return this->cci_handle == des_handle;
			}

		inline size_t CCI_handle:: 
			Recv_large(void*         buffer,
				   size_t 	 count,
				   void*	 context)
			throw(std::runtime_error)
			{
				//forwarding
				return Recv_large(buffer, count, 0);
			}


		inline size_t CCI_handle::
			Send_large(const void*   buffer,
				   size_t 	 count,
				   void*	 context)
			throw(std::runtime_error)
			{
				//forwarding
				return Send_large(buffer, count, 0, context);
			}

		inline size_t CCI_handle:: 
			Recv_large(void*         buffer,
				   size_t 	 count,
				   off64_t	 offset,
				   void*	 context)
			throw(std::runtime_error)
			{
				return Recv_large(nullptr, 0, buffer, count, offset, context);
			}

		inline size_t CCI_handle:: 
			Send_large(const void*   buffer,
				   size_t 	 count,
				   off64_t	 offset,
				   void*	 context)
			throw(std::runtime_error)
			{
				return Send_large(nullptr, 0, buffer, count, offset, context);
			}

		inline cci_endpoint_t* CBB_cci::
			get_endpoint()
		{
			return this->endpoint;
		}

		inline const char* CBB_cci::
			get_my_uri()const
		{
			return this->uri;
		}

		inline CCI_handle& CCI_handle::
			 operator = (const CCI_handle& src)
		{
			this->cci_handle=src.cci_handle; 
			this->local_rma_handle=src.local_rma_handle; 
			memcpy(&this->remote_rma_handle, &src.remote_rma_handle, sizeof(src.remote_rma_handle)); 
			return *this;
		}

		inline void CCI_handle::
			dump_remote_key()const
		{
			_DEBUG("remote key=%lu %lu %lu %lu\n", 
					remote_rma_handle.stuff[0],
					remote_rma_handle.stuff[1],
					remote_rma_handle.stuff[2],
					remote_rma_handle.stuff[3]);
		}

		inline void CCI_handle::
			dump_local_key()const

		{
			_DEBUG("key=%lu %lu %lu %lu\n", 
					local_rma_handle->stuff[0],
					local_rma_handle->stuff[1],	
					local_rma_handle->stuff[2],	
					local_rma_handle->stuff[3]);
		}

		inline void* CCI_handle::
			get_raw_handle()
		{
			const void* ret=cci_handle;
			return const_cast<void*>(ret);
		}

		inline const void* CCI_handle::
			get_raw_handle() const
		{
			return this->cci_handle;
		}

		inline void CCI_handle::
			set_raw_handle(void* cci_handle)
		{
			this->cci_handle=static_cast<cci_connection_t*>(cci_handle);
		}
		
		inline void CCI_handle::
			setup_uri(const char* uri)
		{
			this->uri=uri;
		}

		inline void handle_context::
			reset()
		{
			basic_task::reset();
			this->local_rma_handle=nullptr;
			this->block_ptr=nullptr;
		}
	}
}
#endif
