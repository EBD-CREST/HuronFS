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

block::
block(off64_t	 start_point,
      size_t	 data_size,
      bool 	 dirty_flag,
      bool 	 valid,
      file* 	 file_stat,
      Common::allocator& memory_allocator)
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

