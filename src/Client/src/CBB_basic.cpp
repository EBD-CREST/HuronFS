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

#include "CBB_basic.h"
using namespace CBB::Client;
using namespace CBB::Common;

block_info::
block_info(off64_t 	start_point,
       	   size_t 	size):
	start_point(start_point),
	size(size)
{}

file_meta::
file_meta(ssize_t remote_file_no,
	  size_t block_size,
	  const struct stat* file_stat,
	  SCBB* corresponding_SCBB):
	remote_file_no(remote_file_no),
	open_count(0),
	block_size(block_size),
	file_stat(*file_stat),
	opened_fd(),
	corresponding_SCBB(corresponding_SCBB),
	IOnode_list_cache(),
	block_list(),
	need_update(CLEAN),
	it(nullptr)
{
	_DEBUG("create file_meta %p\n", this);
}

opened_file_info::
opened_file_info(int 		fd,
		 int 		flag,
		 file_meta* 	file_meta_p):
	current_point(0),
	fd(fd),
	flag(flag),
	file_meta_p(file_meta_p)
{
	++file_meta_p->open_count;
	_DEBUG("create open file info %p\n", this);
	_DEBUG("create file meta pointer %p, open count %d\n", file_meta_p, file_meta_p->open_count);
}

opened_file_info::opened_file_info():
	current_point(0),
	fd(0),
	flag(-1),
	file_meta_p(nullptr)
{
	_DEBUG("create file meta pointer %p\n", file_meta_p);
}

opened_file_info::~opened_file_info()
{
	/*if(nullptr != file_meta_p && 0 == --file_meta_p->open_count)
	{
		delete file_meta_p;
	}*/
	file_meta_p->opened_fd.erase(fd);
	_DEBUG("create file meta pointer %p\n", file_meta_p);
}

opened_file_info::opened_file_info(const opened_file_info& src):
	current_point(src.current_point),
	fd(src.fd),
	flag(src.flag),
	file_meta_p(src.file_meta_p)
{}

SCBB::
SCBB(int id,
     comm_handle_t handle):
	IOnode_list(),
	id(id),
	master_handle(*handle)
{}

comm_handle_t SCBB::get_IOnode_fd(ssize_t IOnode_id)	
{
	return &IOnode_list.at(IOnode_id);
}
