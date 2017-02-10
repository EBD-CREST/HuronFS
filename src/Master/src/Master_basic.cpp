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

#include "Master_basic.h"

using namespace CBB::Master;
using namespace CBB::Common;

open_file_info::open_file_info(ssize_t fileno,
		size_t block_size,
		const node_info_pool_t& IOnodes,
		int flag,
		Master_file_stat* file_stat):
	file_no(fileno),
	block_list(),
	IOnodes_set(IOnodes), 
	primary_replica_node(nullptr),
	block_size(block_size),
	open_count(1),
	flag(flag),
	file_status(file_stat)
{}

open_file_info::open_file_info(ssize_t fileno,
		int flag,
		Master_file_stat* file_stat):
	file_no(fileno),
	block_list(),
	IOnodes_set(), 
	primary_replica_node(nullptr),
	block_size(0),
	open_count(1),
	flag(flag),
	file_status(file_stat)
{}

/*void open_file_info::_set_nodes_pool()
{
	for(::const_iterator it=p_node.begin();
			it != p_node.end();++it)
	{
		nodes.insert(it->second);
	}
	return;
}*/

Master_file_stat::Master_file_stat(const struct stat& file_stat, 
		const std::string& filename,
		int exist_flag):
	status(file_stat),
	name(filename),
	external_flag(INTERNAL),
	external_master(0),
	external_name(),
	exist_flag(exist_flag),
	opened_file_info(nullptr),
	full_path(nullptr)
{}

Master_file_stat::Master_file_stat(const struct stat& file_stat, 
		const std::string& filename,
		open_file_info* opened_file_info,
		int exist_flag):
	status(file_stat),
	name(filename),
	external_flag(INTERNAL),
	external_master(0),
	external_name(),
	exist_flag(exist_flag),
	opened_file_info(opened_file_info),
	full_path(nullptr)
{}

Master_file_stat::Master_file_stat():
	status(),
	name(),
	external_flag(INTERNAL),
	external_master(0),
	external_name(),
	exist_flag(),
	opened_file_info(),
	full_path(nullptr)
{}

node_info::node_info(ssize_t id,
		const std::string& uri,
		size_t total_memory,
		comm_handle_t handle):
	handle(),
	node_id(id),
	uri(uri), 
	stored_files(file_no_pool_t()), 
	avaliable_memory(total_memory), 
	total_memory(total_memory)
{
	memcpy(&(this->handle), handle, sizeof(this->handle));
}
