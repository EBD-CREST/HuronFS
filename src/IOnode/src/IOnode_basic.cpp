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
	swapout_flag(INBUFFER),
	writing_back(UNSET),
	receiving_data(UNSET),
	exist_flag(file_stat->exist_flag),
	file_stat(file_stat),
	data_rwlock(),
	meta_lock(),
	postponed_operation(CLEAN),
	writeback_page(nullptr),
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
	if(nullptr != this->writeback_page)
	{
		writeback_page->reset_myself();
		_DEBUG("free write back page %p\n", this->writeback_page);
	}

	if(nullptr != _elem)
	{
		memory_allocator.free(_elem);
	}
}

block::
block(	const block & src):
	data_size(src.data_size),
	block_size(BLOCK_SIZE),
	data(src.data), 
	start_point(src.start_point),
	dirty_flag(src.dirty_flag),
	valid(src.valid),
	swapout_flag(INBUFFER),
	writing_back(src.writing_back),
	receiving_data(src.receiving_data),
	exist_flag(src.exist_flag),
	file_stat(nullptr),
	data_rwlock(),
	meta_lock(),
	postponed_operation(CLEAN),
	writeback_page(src.writeback_page),
	memory_allocator(src.memory_allocator),
	_elem(src._elem)
{};

file::
file(const char	 *path,
     int	 exist_flag,
     ssize_t 	 file_no):
	file_path(path),
	exist_flag(exist_flag),
	close_flag(OPEN),
	dirty_flag(CLEAN),
	postponed_operation(CLEAN),
	dirty_pages_count(0),
	main_flag(SUB_REPLICA),
	file_no(file_no),
	blocks(),
	IOnode_pool(),
	read_remote_fd(-1),
	write_remote_fd(-1),
	_lock()

{
	if(EXISTING == this->exist_flag)
	{
		_DEBUG("existing\n");
	}
}

file::
~file()
{
	_DEBUG("delete file %s\n", file_path.c_str());
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
	if(nullptr != _elem)
	{
		memory_allocator.free(_elem);
	}

	_elem=memory_allocator.allocate();
	data=_elem->get_memory();
	valid=VALID;
	return block_size;
}

size_t block::
free_memory()
{
	if(DIRTY == this->dirty_flag)
	{
		_DEBUG("try to free a dirty block\n");
		return 0;
	}
	else if(VALID != this->valid)
	{
		_DEBUG("try to free a invalid block\n");
		return 0;
	}
	else if(nullptr == this->data)
	{
		_DEBUG("data is empty");
		return 0;
	}
	else
	{
		_DEBUG("free %p\n", this->data);
		memory_allocator.free(_elem);
		this->data=nullptr;
		this->_elem=nullptr;
		this->writeback_page=nullptr;

		return this->block_size;
	}
}
