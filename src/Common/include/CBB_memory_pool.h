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
#ifndef CBB_MEMORY_POOL_H_
#define CBB_MEMORY_POOL_H_

#include <exception>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include "CBB_internal.h"

namespace CBB
{
	namespace Common
	{
		struct memory_elem
		{
		public:
			memory_elem(size_t size)throw(std::bad_alloc);
			~memory_elem();
			void* get_memory();
		public:
			void* 		memory;
			memory_elem* 	next;
		};

		class memory_pool
		{
		public:
			memory_pool(size_t size_of_elem, 
				int total_count, int pre_alloc_count)throw(std::bad_alloc);
			memory_pool();
			void setup(size_t size_of_elem, 
				int total_count, int pre_alloc_count)throw(std::bad_alloc);
			~memory_pool();
			void free(memory_elem* memory);
			memory_elem* allocate()throw(std::bad_alloc);
			bool has_memory_left(size_t)const;
			size_t get_available_memory_size()const;
			size_t get_total_memory_size()const;
		private:
			size_t allocate_more_memory()throw(std::bad_alloc);
		private:
			memory_elem* head;
			size_t	     available_memory;
			size_t       allocated_size;
			size_t	     block_size;
			size_t	     step_size;
			size_t	     total_size;
		};

		typedef memory_pool allocator;


		inline void* memory_elem::
			get_memory()
		{
			return memory;
		}

		inline bool memory_pool::
			has_memory_left(size_t size)const
		{
			return available_memory >= size || 
				total_size >= allocated_size + block_size;
		}

		inline size_t memory_pool::
			get_available_memory_size()const
		{
			return this->available_memory;
		}

		inline size_t memory_pool::
			get_total_memory_size()const
		{
			return this->total_size;
		}
	}
}

#endif
