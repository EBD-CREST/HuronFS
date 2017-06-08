/*
 * Copyright (c) 2017, Tokyo Institute of Technology
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
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

#ifndef IONODE_BASIC_H_
#define IONODE_BASIC_H_

#include <map>
#include <list>

#include "Comm_basic.h"
#include "CBB_swap.h"

namespace CBB
{
	namespace IOnode
	{
		struct memory_elem
		{
		public:
			memory_elem(size_t size)throw(std::bad_alloc);
			~memory_elem();
			void* get_data();
		public:
			void* 		data;
			size_t		size;
			memory_elem* 	next;
		};

		class memory_pool
		{
		public:
			memory_pool(size_t size_of_elem, int count)throw(std::bad_alloc);
			memory_pool();
			void setup(size_t size_of_elem, int count)throw(std::bad_alloc);
			~memory_pool();
			void free(memory_elem* memory);
			memory_elem* allocate()throw(std::bad_alloc);
			bool has_memory_left(size_t)const;
			size_t get_available_memory_size()const;
		private:
			memory_elem* head;
			size_t	     available_memory;
		};

		typedef memory_pool allocator;

		struct file;
		struct block
		{
			block(off64_t 	start_point,
				size_t 	block_size,
				bool 	dirty_flag,
				bool 	valid,
				file* 	file_stat,
				allocator& _allocator)
				throw(std::bad_alloc);
			~block();
			block(const block&);

			size_t allocate_memory()throw(std::bad_alloc);
			bool need_allocation()const;
			size_t free_memory();
			int lock();
			int unlock();

			size_t 			data_size;
			size_t			block_size;
			void* 			data;
			off64_t 		start_point;
			bool 			dirty_flag;
			bool 			valid;
			int 			exist_flag;
			file*			file_stat;
			//Common::remote_task* 	write_back_task;
			pthread_mutex_t 	locker;
			//remote handler will delete this struct if TO_BE_DELETED is setted
			//this appends when user unlink file while remote handler is writing back
			bool 			TO_BE_DELETED;
			Common::
			access_page<block>*	writeback_page;
			allocator&		memory_allocator;
			memory_elem*		_elem;
		};

		typedef std::map<off64_t, block*> block_info_t; //map: start_point : block*
		typedef std::map<ssize_t, Common::comm_handle_t> handle_ptr_pool_t; //map node id: handle

		struct file
		{
			file(const char *path,
			     int exist_flag, 
			     ssize_t file_no);
			~file();

			std::string 		file_path;
			int 			exist_flag;
			bool 			dirty_flag;
			int 			main_flag; //main replica indicator
			ssize_t 		file_no;
			block_info_t  		blocks;
			handle_ptr_pool_t 	IOnode_pool;
		};

		inline int block::
			lock()
		{
			return pthread_mutex_lock(&locker);
		}

		inline int block::
			unlock()
		{
			return pthread_mutex_unlock(&locker);
		}

		inline bool block::
			need_allocation()const
		{
			return INVALID == this->valid ||
				nullptr == data;
		}

		inline size_t block::
			free_memory()
		{
			if(DIRTY != this->dirty_flag  && 
				VALID == this->valid  &&
				nullptr != this->data)
			{
				memory_allocator.free(_elem);
				this->data=nullptr;
				return this->block_size;
			}
			else
			{
				return 0;
			}
		}

		inline void* memory_elem::
			get_data()
		{
			return data;
		}

		inline bool memory_pool::
			has_memory_left(size_t size)const
		{
			return available_memory >= size;
		}

		inline size_t memory_pool::
			get_available_memory_size()const
		{
			return this->available_memory;
		}
	}
}
#endif
