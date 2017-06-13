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

#include "CBB_memory_pool.h"

using namespace CBB::Common;
using namespace std;

memory_elem::
memory_elem(size_t size)
throw(std::bad_alloc):
	data(malloc(size)),
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
	block_size=size_of_elem;

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
	head(nullptr),
	available_memory(0),
	block_size(0)
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
		available_memory-=block_size;
		_DEBUG("available_memory %ld\n", available_memory);

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
	available_memory+=block_size;
	_DEBUG("available_memory %ld\n", available_memory);
}

