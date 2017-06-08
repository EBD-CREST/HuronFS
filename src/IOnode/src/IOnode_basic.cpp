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

#include "IOnode_basic.h"

using namespace CBB::IOnode;
using namespace CBB::Common;
using namespace std;

memory_elem::
memory_elem(size_t size)
throw(std::bad_alloc):
	data(malloc(size)),
	size(size),
	next(nullptr)
{
	if(nullptr == data)
	{
		throw std::bad_alloc();
	}
}

memory_elem::
~memory_elem()
{
	free(data);
}

void memory_pool::
setup(size_t size_of_elem, int count)
throw(std::bad_alloc)
{
	available_memory=size_of_elem*count;
	while(count--)
	{
		memory_elem* new_elem=new memory_elem(size_of_elem);
		new_elem->next=head;
		head=new_elem;
	}
}

memory_pool::
memory_pool(size_t size_of_elem, int count)
throw(std::bad_alloc):
	head(nullptr)
{
	setup(size_of_elem, count);
}

memory_pool::
memory_pool():
	head(nullptr)
{}

memory_pool::
~memory_pool()
{
	while(nullptr != head)
	{
		memory_elem* next=head->next;
		delete head;
		head=next;
	}
}
		
memory_elem* memory_pool::
allocate()
throw(std::bad_alloc)
{
	if(nullptr != head)
	{
		memory_elem* ret=head;
		head=head->next;
		available_memory-=ret->size;

		return ret;
	}
	else
	{
		throw std::bad_alloc();
	}
}

void memory_pool::
free(memory_elem* memory)
{
	memory->next=head;
	head=memory;
	available_memory+=memory->size;
}

block::
block(off64_t	 start_point,
      size_t	 data_size,
      bool 	 dirty_flag,
      bool 	 valid,
      file* 	 file_stat,
      allocator& memory_allocator)
throw(std::bad_alloc):
	data_size(data_size),
	block_size(BLOCK_SIZE),
	data(nullptr),
	start_point(start_point),
	dirty_flag(dirty_flag),
	valid(INVALID),
	file_stat(file_stat),
	locker(),
	TO_BE_DELETED(CLEAN),
	memory_allocator(memory_allocator),
	_elem(nullptr)
{
	if(INVALID != valid)
	{
		this->allocate_memory();
	}
}

block::
~block()
{
	memory_allocator.free(_elem);
}

block::
block(	const block & src):
	data_size(src.data_size),
	data(src.data), 
	start_point(src.start_point),
	dirty_flag(src.dirty_flag),
	valid(src.valid),
	file_stat(nullptr),
	locker(),
	TO_BE_DELETED(CLEAN),
	memory_allocator(src.memory_allocator),
	_elem(src._elem)
{};

file::
file(const char	 *path,
     int	 exist_flag,
     ssize_t 	 file_no):
	file_path(path),
	exist_flag(exist_flag),
	dirty_flag(CLEAN),
	main_flag(SUB_REPLICA),
	file_no(file_no),
	blocks(),
	IOnode_pool()
{}

file::
~file()
{
	block_info_t::const_iterator end=blocks.end();
	for(block_info_t::iterator block_it=blocks.begin();
			block_it!=end;++block_it)
	{
		delete block_it->second;
	}
}

size_t block::
allocate_memory()
throw(std::bad_alloc)
{
	if(nullptr != data)
	{
		memory_allocator.free(_elem);
	}

	_elem=memory_allocator.allocate();
	data=_elem->get_data();
	valid=VALID;
	return block_size;
}

