/*
 * IOnode.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */

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

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <sys/epoll.h>
#include <regex>
#include <fcntl.h>

#include "IOnode.h"
#include "CBB_const.h"

using namespace CBB::IOnode;
using namespace CBB::Common;
using namespace std;

const char *IOnode::IONODE_MOUNT_POINT="HUFS_IONODE_MOUNT_POINT";
const char *IOnode::IONODE_MEMORY_LIMIT="HUFS_IONODE_MEMORY_LIMIT";

IOnode::
IOnode(	const string&	master_ip,
	int		master_port) 
throw(std::runtime_error):
	Server(IONODE_QUEUE_NUM, IONODE_PORT), 
	CBB_data_sync(),
	CBB_swap<block>(),
	my_node_id(-1),
	_files(file_t()),
	_current_block_number(0), 
	_MAX_BLOCK_NUMBER(MAX_BLOCK_NUMBER), 
	master_uri(),
	master_handle(),
	my_handle(),
	_mount_point(std::string()),
	IOnode_handle_pool(),
	writeback_queue(),
	writeback_status(IDLE),
	dirty_pages(CLEAN),
	memory_pool_for_blocks(),
	remove_file_list(),
	open_file_count(0)
{
	const char *IOnode_mount_point=getenv(IONODE_MOUNT_POINT);
	const char *IOnode_memory_limit=getenv(IONODE_MEMORY_LIMIT);
	master_uri=master_ip;
	size_t total_memory=MEMORY;

	if( nullptr == IOnode_mount_point)
	{
		throw runtime_error(
			"please set IONODE_MOUNT_POINT environment value");
	}

	_mount_point=string(IOnode_mount_point);

	if( nullptr != IOnode_memory_limit)
	{
		total_memory=_set_memory_limit(IOnode_memory_limit);
	}

	memory_pool_for_blocks.setup(BLOCK_SIZE, total_memory/BLOCK_SIZE, IONODE_PRE_ALLOCATE_MEMORY_COUNT);

	_setup_queues();

	try
	{
		Server::_init_server();
	}
	catch(runtime_error& e)
	{
		throw;
	}
}

int IOnode::
_setup_queues()
{
	CBB_data_sync::
	set_queues(&_communication_input_queue.at(IONODE_DATA_SYNC_QUEUE_NUM),
	  	  &_communication_output_queue.at(IONODE_DATA_SYNC_QUEUE_NUM));
	return SUCCESS;
}

IOnode::
~IOnode()
{
	_DEBUG("I am going to shut down\n");
	_unregister();
}

int IOnode::
start_server()
{
	Server::start_server();
	CBB_data_sync::start_listening();
	if(-1  ==  
	(my_node_id=_register(
		&_communication_input_queue, 
		&_communication_output_queue)))
	{
		throw runtime_error("Get Node Id Error"); 
	}
	while(true)
	{
		sleep(1000000);
	}
	return SUCCESS;
}

int IOnode::
_unregister()
{
	/*extended_IO_task* output=output_queue->allocate_tmp_node();
	output->set_handle(_master_handle);
	output->push_back(CLOSE_CLIENT);
	output->queue_enqueue();*/
	return SUCCESS;
}

ssize_t IOnode::
_register(communication_queue_array_t* input_queue,
	  communication_queue_array_t* output_queue)
throw(std::runtime_error)
{ 
	_LOG("start registration\n");

	extended_IO_task* output=get_connection_task();
	output->setup_new_connection(master_uri.c_str(), MASTER_PORT);
	output->push_back(REGISTER);
	output->push_back(memory_pool_for_blocks.get_available_memory_size());
	connection_task_enqueue(output);

	extended_IO_task* response=
		get_connection_response();

	if(response->has_error())
	{
		throw std::runtime_error("connect to master failed");
	}

	ssize_t id=-1;
	response->pop(id);
	master_handle=*response->get_handle();

	connection_task_dequeue(response);
	_LOG("registration finished, id=%ld\n", id);
	return id; 
}

/*int IOnode::_parse_new_request(int sockfd,
		const struct sockaddr_in& client_addr)
{
	int request, ans=SUCCESS; 
	Recv(sockfd, request); 
	switch(request)
	{
	case SERVER_SHUT_DOWN:
		ans=SERVER_SHUT_DOWN; break; 
	case READ_FILE:
		_send_data(sockfd);break;
	case NEW_CLIENT:
		_regist_new_client(sockfd);break;
	default:
		break; 
	}
	return ans; 
}*/

//request from master
int IOnode::
_parse_request(extended_IO_task* new_task)
{
	int request, ans=SUCCESS; 
	new_task->pop(request); 
	switch(request)
	{
	case NEW_CLIENT:
		_register_new_client(new_task);	break;
	case READ_FILE:
		_send_data(new_task);		break;
	case WRITE_FILE:
		_receive_data(new_task);	break;
	case OPEN_FILE:
		_open_file(new_task);		break;
	case APPEND_BLOCK:
		_append_new_block(new_task);	break;
	case RENAME:
		_rename(new_task);		break;
	case FLUSH_FILE:
		_flush_file(new_task);		break;
	case CLOSE_FILE:
		_close_file(new_task);		break;
	case CLOSE_CLIENT:
		_close_client(new_task);	break;
	case TRUNCATE:
		_truncate_file(new_task);	break;
	case UNLINK:
		_unlink(new_task);		break;
	case HEART_BEAT:
		_heart_beat(new_task);		break;
	case NODE_FAILURE:
		_node_failure(new_task);	break;
	case DATA_SYNC:
		_get_sync_data(new_task);	break;
	case NEW_IONODE:
		_register_new_IOnode(new_task);	break;
	case PROMOTED_TO_PRIMARY_REPLICA:
		_promoted_to_primary(new_task);	break;
	case REPLACE_REPLICA:
		_replace_replica(new_task);	break;
	case REMOVE_IONODE:
		_remove_IOnode(new_task);	break;
	default:
		_DEBUG("unrecognized command %d\n",
			request); break; 
	}

	return ans; 
}

int IOnode::
_promoted_to_primary(extended_IO_task* new_task)
{
	ssize_t file_no		=0;
	ssize_t new_node_id	=0;
	int 	ret		=SUCCESS;
	_LOG("promoted to primary replica\n");
	
	new_task->pop(file_no);
	new_task->pop(new_node_id);
	try
	{
		file& file_info=_files.at(file_no);

		if(my_node_id != new_node_id)
		{
			_get_replica_node_info(new_task, file_info);
			add_data_sync_task(DATA_SYNC_INIT,
					&file_info, 0, 0, -1, 0,
					&IOnode_handle_pool.at(new_node_id), nullptr);
		}
	}
	catch(std::runtime_error&e) {
		_DEBUG("try to connect to IOnode failed!!\n");
		ret=FAILURE;
	}
	catch(std::out_of_range &e)
	{
		_DEBUG("no such file !!\n");
		ret=FAILURE;
	}
	return ret;
}

int IOnode::
_replace_replica(extended_IO_task* new_task)
{
	ssize_t file_no		=0;
	int 	ret		=SUCCESS;
	ssize_t old_node_id	=0;
	_LOG("replace replica\n");

	new_task->pop(file_no);
	try
	{
		file& file_info=_files.at(file_no);

		new_task->pop(old_node_id);	

		file_info.IOnode_pool.erase(old_node_id);
		_get_IOnode_info(new_task, file_info);
	}
	catch(std::runtime_error&e)
	{
		_DEBUG("out of range !!\n");
		ret=FAILURE;
	}
	return ret;
}

int IOnode::
_remove_IOnode(extended_IO_task* new_task)
{
	ssize_t node_id	=0;

	new_task->pop(node_id);
	node_handle_pool_t::iterator it=IOnode_handle_pool.find(node_id);
	_LOG("remove IOnode id=%ld\n", node_id);

	if(end(IOnode_handle_pool) != it)
	{
		it->second.Close();
		IOnode_handle_pool.erase(it);
	}
	return SUCCESS;
}

int IOnode::
_register_new_IOnode(extended_IO_task* new_task)
{
	ssize_t new_IOnode_id	=0;
	comm_handle_t handle	=new_task->get_handle();

	new_task->pop(new_IOnode_id);
	_LOG("register new IOnode %ld \n", new_IOnode_id);

	IOnode_handle_pool.insert(std::make_pair(new_IOnode_id, *handle));
	extended_IO_task* output=init_response_task(new_task);
	output->push_back(SUCCESS);

	output_task_enqueue(output);
	return SUCCESS;
}

int IOnode::
_send_data(extended_IO_task* new_task)
{
	ssize_t file_no		=0;
	off64_t start_point	=0;
	off64_t offset		=0;
	size_t  size		=0;
	ssize_t remaining_size	=0;
	int 	ret		=SUCCESS;

	//recording
	start_recording(this);

	new_task->pop(file_no);
	new_task->pop(start_point);
	new_task->pop(offset);
	new_task->pop(size);
	_LOG("request for sending data\n");
	_LOG("file_no=%ld, start_point=%ld, offset=%ld, size=%lu\n",
			file_no, start_point, offset, size);

	extended_IO_task* output=init_response_task(new_task);
	try
	{
		file& _file=_files.at(file_no);
		const std::string& path = _file.file_path;
		remaining_size=size;
		size_t IO_size=offset+size>BLOCK_SIZE?BLOCK_SIZE-offset:size;
		_DEBUG("file path=%s\n", path.c_str());

		output->init_response_large_transfer(new_task, RMA_WRITE);
		output->push_back(SUCCESS);
		while(remaining_size > 0)
		{
			_DEBUG("remaining_size=%ld, start_point=%ld\n",
					remaining_size, start_point);
			block* requested_block=nullptr;
			try
			{
				requested_block=_file.blocks.at(start_point);
			}
			catch(std::out_of_range&e )
			{
				_DEBUG("out of range\n");
				break;
			}
			requested_block->metalock();
			requested_block->dataRdlock();

			requested_block->file_stat=&_file;
			if(requested_block->need_allocation())
			{
				allocate_memory_for_block(requested_block);
				//we read the data from remote back in following two situations
				//the file exists in the remote as inputs
				//the chunk has been swaped out
				if(EXISTING == requested_block->exist_flag ||
					SWAPPED_OUT == requested_block->swapout_flag)
				{
					if(SWAPPED_OUT == requested_block->swapout_flag)
					{
						_LOG("reswap in\n");
					}
					_read_from_storage(path, _file, requested_block);
					requested_block->swapout_flag=INBUFFER;
				}
			}

			update_access_order(requested_block, CLEAN);
			
			start_point+=BLOCK_SIZE;
			output->push_send_buffer(
				reinterpret_cast<char*>(
					requested_block->data)+offset,
				IO_size, requested_block);
			remaining_size-=IO_size;
			IO_size=MIN(remaining_size,
				static_cast<ssize_t>(BLOCK_SIZE));
			offset=0;
			requested_block->metaunlock();
		}
		ret=SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		_DEBUG("file not found\n");
		output->push_back(FILE_NOT_FOUND);
		ret=FAILURE;
	}
	output_task_enqueue(output);

	/*end_recording(this, 0, READ_FILE);
	this->print_log_debug("r", "send data", start_point, size);
	this->print_log("r", "send data", start_point, size);*/

	return ret;

}

int IOnode::
_receive_data(extended_IO_task* new_task)
{
	ssize_t file_no		=0;
	off64_t start_point	=0;
	off64_t offset		=0;
	size_t 	size		=0;
	ssize_t remaining_size	=0;
	int 	ret		=SUCCESS;


	//recording
	start_recording(this);
	new_task->pop(file_no);
	new_task->pop(start_point);
	new_task->pop(offset);
	new_task->pop(size);
	_LOG("request for receiving data\n");
	_LOG("file_no=%ld, start_point=%ld, offset=%ld, size=%lu\n",
			file_no, start_point, offset, size);

	new_task->init_for_large_transfer(RMA_READ);

	try
	{
		remaining_size=size;
		file& _file=_files.at(file_no);
		block_info_t &blocks=_file.blocks;
		_DEBUG("file path=%s\n", _file.file_path.c_str());

		size_t receive_size=
			(offset + size) > BLOCK_SIZE?BLOCK_SIZE-offset:size;
		//the tmp variables for receiving data
		off64_t tmp_offset=offset;
		off64_t tmp_start_point=start_point;

		while(0 < remaining_size)
		{
			//start_recording(this);

			_DEBUG("receive_data start point %ld, offset %ld, receive size %ld, remaining size %ld\n",
					tmp_start_point, tmp_offset,
					receive_size, remaining_size);
			ssize_t IOsize=update_block_data(blocks,
					_file, tmp_start_point,
					tmp_offset, receive_size,
					new_task);

			remaining_size	-=IOsize;
			receive_size	=MIN(
					remaining_size,
					static_cast<ssize_t>(BLOCK_SIZE));
			tmp_start_point	+=IOsize+tmp_offset;
			tmp_offset	=0;

		}
		_file.dirty_flag=DIRTY;

		_sync_data(_file, start_point, 
			offset, size, new_task->get_receiver_id(), 
			new_task->get_handle(), new_task->get_send_buffer());
		ret=SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		_DEBUG("File is not found\n");
		extended_IO_task* response=init_response_task(new_task);
		response->push_back(FILE_NOT_FOUND);
		output_task_enqueue(response);
		ret=FAILURE;
	}

	/*end_recording(this, 0, WRITE_FILE);
	this->print_log_debug("w", "get receive data", start_point, size);
	this->print_log("w", "get receive data", start_point, size);*/

	return ret;
}

size_t IOnode::
update_block_data(block_info_t&		blocks,
		  file&			file, 
		  off64_t 		start_point,
		  off64_t 		offset,
		  size_t 		size,
		  extended_IO_task* 	new_task)
{
	size_t ret	=0;
	block* _block	=nullptr;

	try
	{
		_block=blocks.at(start_point);
	}
	catch(std::out_of_range &e)
	{
		_block=blocks.insert(std::make_pair(start_point, 
				create_new_block(start_point, size, CLEAN, 
				INVALID, &file))).first->second;
	}

	//unlock after finished receiving
	_block->dataWrlock();
	_block->metalock();
	_block->set_to_receive_data();

	if(_block->need_allocation())
	{
		allocate_memory_for_block(_block);
		if(EXISTING == file.exist_flag ||
			SWAPPED_OUT == _block->swapout_flag)
		{
			if(SWAPPED_OUT == _block->swapout_flag)
			{
				_LOG("reswap in\n");
			}
			_block->data_size=
				_read_from_storage(file.file_path, file, _block);
			_block->swapout_flag=INBUFFER;
		}
		_block->valid = VALID;
	}

	if(offset + size > _block->block_size)
	{
		size = _block->block_size - offset;
	}

	if(_block->data_size < offset+size)
	{
		_block->data_size = offset+size;
	}

	_DEBUG("receive data at %p, offset %ld, size=%ld\n", _block->data, offset, size);
	ret=new_task->get_received_data(
			static_cast<unsigned char*>(_block->data)+offset,
			size, _block);

	_DEBUG("%p is dirty now\n", _block);

	//receiving block data
	_DEBUG("start receiving data %p\n", _block);

	if(DIRTY != _block->dirty_flag)
	{
		_block->dirty_flag=DIRTY;
		file.add_dirty_pages(1);
	}

	update_access_order(_block, DIRTY);
	_block->metaunlock();

	return ret;
}

int IOnode::
_sync_data(file& 	 file_info, 
	   off64_t	 start_point, 
	   off64_t	 offset,
	   ssize_t	 size,
	   int		 receiver_id,
	   comm_handle_t handle,
	   send_buffer_t* send_buffer)
{
	_DEBUG("offset %ld\n", offset);
	return add_data_sync_task(DATA_SYNC_WRITE, &file_info,
			start_point, offset, size, receiver_id, handle, send_buffer);
}

int IOnode::
_open_file(extended_IO_task* new_task)
{
	ssize_t file_no		=0; 
	int 	flag		=0;
	char 	*path_buffer	=nullptr; 
	int 	exist_flag	=0;
	int 	count		=0;

	new_task->pop(file_no);
	new_task->pop(flag);
	new_task->pop(exist_flag);
	new_task->pop_string(&path_buffer); 
	_LOG("openfile fileno=%ld, path=%s\n", file_no, path_buffer);
	file_t::iterator file_it=_files.find(file_no);
	
	if(end(_files) == file_it)
	{
		file& _file=_files.insert(make_pair(file_no, 
			file(path_buffer, exist_flag,
				file_no))).first->second;
		//get blocks
		new_task->pop(count);
		for(int i=0;i<count;++i)
		{
			_append_block(new_task, _file);
		}
		//get replica ips
		_get_replica_node_info(new_task, _file);
	}
	else
	{
		file_it->second.close_flag=OPEN;
	}

	return SUCCESS; 
}

int IOnode::
_get_replica_node_info(extended_IO_task* new_task,
	       	       file& 		 _file)
{
	int count	=0;
	int main_flag	=0;

	new_task->pop(main_flag);
	new_task->pop(count);
	for(int i=0;i<count;++i)
	{
		_get_IOnode_info(new_task, _file);
	}
	return SUCCESS;
}

int IOnode::
_get_IOnode_info(extended_IO_task* new_task, 
		 file& 		   _file)
{
	comm_handle_t 
		new_handle	=nullptr;
	char* 	node_ip		=nullptr;
	ssize_t node_id		=0;

	new_task->pop(node_id);
	new_task->pop_string(&node_ip);
	node_handle_pool_t::iterator it;

	if(my_node_id != node_id)
	{
		_DEBUG("try to connect to %s\n", node_ip);

		if(end(IOnode_handle_pool) == (it=IOnode_handle_pool.find(node_id)))
		{
			new_handle=_connect_to_new_IOnode(node_id, my_node_id, node_ip);
		}
		else
		{
			new_handle=&(it->second);
		}
		_file.IOnode_pool.insert(std::make_pair(node_id, new_handle));
	}
	else
	{
		_DEBUG("new node is myself\n");
	}
	return SUCCESS;
}

comm_handle_t IOnode::
_connect_to_new_IOnode(ssize_t 		destination_node_id,
		       ssize_t 		my_node_id,
		       const char* 	node_ip)
{
	int ret;
	_DEBUG("connect to destination node id %ld\n", destination_node_id);

	extended_IO_task* output=get_connection_task();
	output->setup_new_connection(node_ip, IONODE_PORT);
	output->push_back(NEW_IONODE);
	output->push_back(my_node_id);
	output_task_enqueue(output);

	extended_IO_task* response=
		get_connection_response();
	response->pop(ret);

	comm_handle_t IOnode_handle=
		&(IOnode_handle_pool.insert(
		make_pair(destination_node_id, 
			*response->get_handle())).first->second);
	connection_task_dequeue(response);
	return IOnode_handle;
}

int IOnode::
_close_file(extended_IO_task* new_task)
{
	ssize_t file_no=0;
	new_task->pop(file_no);
	file_t::iterator _file=_files.find(file_no);

	if(end(_files) != _file)
	{
		_file->second.lock();
		if(TO_BE_DELETED != _file->second.postponed_operation &&
		(-1 != _file->second.read_remote_fd ||
		 -1 != _file->second.write_remote_fd))
		{
		//_LOG("write remote fd %d\n", _file->second.write_remote_fd);
		//_LOG("read remote fd %d\n", _file->second.read_remote_fd);
		/*CBB_remote_task::add_remote_task(
			CBB_REMOTE_CLOSE, &(_file->second));*/
			_file->second.postponed_operation=TO_BE_CLOSED;
		}
		_file->second.unlock();
	}

	//block_info_t &blocks=_file.blocks;
	//_DEBUG("close file, path=%s\n", _file.file_path.c_str());
	/*for(block_info_t::iterator it=blocks.begin();
	  it != blocks.end();++it)
	  {
	  block* _block=it->second;
	  _block->lock();

	  if((DIRTY == _block->dirty_flag 	&&
	  nullptr == _block->write_back_task) 	||
	  (CLEAN == _block->dirty_flag &&
	  nullptr != _block->write_back_task))
	  {
	  }
	  _block->unlock();
	  }*/
	return SUCCESS;
}

int IOnode::
_rename(extended_IO_task* new_task)
{
	ssize_t file_no		=0; 
	char 	*new_path	=nullptr; 
	new_task->pop(file_no);
	new_task->pop_string(&new_path);
	_DEBUG("rename file_no =%ld, new_path=%s\n", file_no, new_path);

	try{
		file& _file=_files.at(file_no);
		_file.file_path=std::string(new_path);
	}
	catch(std::out_of_range& e)
	{
		_DEBUG("file unfound");
	}
	return SUCCESS; 
}

void IOnode::
_append_block(extended_IO_task* new_task,
	      file& 		file_stat)
throw(std::runtime_error)
{
	off64_t start_point	=0;
	size_t 	data_size	=0;
	block*	new_block	=nullptr;

	new_task->pop(start_point); 
	new_task->pop(data_size); 
	_DEBUG("append request from Master\n");
	_DEBUG("start_point=%lu, data_size=%lu\n", start_point, data_size);
	if(nullptr == (new_block=create_new_block(start_point,
					   data_size,
					   CLEAN,
					   INVALID,
					   &file_stat)))
	{
		perror("allocate");
		throw std::runtime_error("appen_block");
	}
	file_stat.blocks.insert(std::make_pair(start_point, new_block));
	return ;
}

int IOnode::
_append_new_block(extended_IO_task* new_task)
{
	int 	count		=0;
	ssize_t file_no		=0;
	off64_t start_point	=0;
	size_t 	data_size	=0;
	
	new_task->pop(file_no);
	new_task->pop(count);
	try
	{
		file& _file=_files.at(file_no);
		block_info_t &blocks=_file.blocks;
		for(int i=0;i<count;++i)
		{
			new_task->pop(start_point);
			new_task->pop(data_size);
			blocks.insert(std::make_pair(
			start_point, 
			create_new_block(start_point, data_size, 
				CLEAN, INVALID, &_file)));
		}
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		return FAILURE;
	}
}

int IOnode::
_open_remote_file(
	file* 			file_stat,
	const std::string&	real_path,
	int			mode)
{
	int open_mode=0;
	int* fd=nullptr;

	file_stat->lock();

	switch(mode)
	{
		case READ_FILE:
			fd=&(file_stat->read_remote_fd);
			open_mode=O_RDONLY;
			break;	
		case WRITE_FILE:
			fd=&(file_stat->write_remote_fd);
			open_mode=O_WRONLY|O_CREAT;
			break;
	}

	//start_recording(this);
	if( -1 == *fd)
	{
		if(!this->open_too_many_files())
		{
			this->open_file_count += 1;
			*fd = open64(real_path.c_str(),open_mode, 0600);
			if(-1 == *fd)
			{
				perror("Open File");
				_DEBUG("path %s\n", real_path.c_str());
				throw std::runtime_error("Open File Error\n");
			}
			_DEBUG("opening %d path %s\n", *fd, real_path.c_str());
		}
		else
		{
			throw std::runtime_error("Open too many files\n");
		}

	}
	file_stat->unlock();

	return *fd;
}

int IOnode::
_close_remote_file(file* file_stat,
		   int   mode)
{
	if(mode == WRITE_FILE)
	{
		file_stat->remove_dirty_pages(1);
	}

	file_stat->lock();

	/*if(!open_too_many_files() &&
		TO_BE_CLOSED != file_stat->postponed_operation &&
		TO_BE_DELETED != file_stat->postponed_operation)
	{
		file_stat->unlock();
		return SUCCESS;
	}*/

	_DEBUG("close remote file %s\n", file_stat->file_path.c_str());
	if(-1 != file_stat->read_remote_fd)
	{
		close(file_stat->read_remote_fd);
		this->open_file_count -= 1;
		file_stat->read_remote_fd=-1;
	}

	if(-1 != file_stat->write_remote_fd)
	{
		close(file_stat->write_remote_fd);
		this->open_file_count -= 1;
		file_stat->write_remote_fd=-1;
	}

	file_stat->unlock();
	return SUCCESS;
}

size_t IOnode::
_read_from_storage(const string& 	path,
		   file&		file_stat,
		   block* 		block_data)
throw(std::runtime_error)
{
	off64_t 	start_point	=block_data->start_point;
	std::string 	real_path	=_get_real_path(path);
	ssize_t		ret		=SUCCESS; 
	struct iovec 	vec;
	char 		*buffer		=reinterpret_cast<char*>(
			block_data->data);
	size_t 		size		=block_data->data_size;
	int 		fd		=_open_remote_file(&file_stat,
					real_path, READ_FILE);

	_DEBUG("%s\n", real_path.c_str());

	if(-1  == lseek(fd, start_point, SEEK_SET))
	{
		perror("Seek"); 
		_DEBUG("path %s\n", real_path.c_str());
		return 0;
	}

	vec.iov_base=block_data->data;
	vec.iov_len=size;
	while(0 != size && 0!=(ret=readv(fd, &vec, 1)))
	{
		if(ret  == -1)
		{
			if(EINTR == errno)
			{
				continue; 
			}
			perror("read"); 
			break; 
		}
		vec.iov_base=buffer+ret;
		size-=ret;
		vec.iov_len=size;
	}
	_close_remote_file(&file_stat, READ_FILE);

	return block_data->data_size-size;
}

size_t IOnode::
_write_to_storage(block* block_data, const char* mode)
throw(std::runtime_error)
{
	block_data->metalock();
	_DEBUG("checking block %p\n", block_data);
	if(block_data->is_receiving_data())
	{
		block_data->metaunlock();
		return 0;/*let the operator to rechoose*/
	}

	_LOG("start writing back %s of %p\n", mode, block_data);

	block_data->dirty_flag=CLEAN;

	string real_path	=_get_real_path(block_data->file_stat->file_path);
	off64_t     start_point	=block_data->start_point;
	size_t	    size	=block_data->data_size, len=size;
	const char* buf		=static_cast<const char*>(block_data->data);
	ssize_t	    ret		=0;
	file*	    file_stat	=block_data->file_stat;
	int	    fd		=_open_remote_file(file_stat, real_path,
					WRITE_FILE);

	if(TO_BE_DELETED == block_data->postponed_operation)
	{
		block_data->metaunlock();
		_close_remote_file(file_stat, DELETE_FILE);
		return 0;
	}

	block_data->writing_back=SET;;
	block_data->dataRdlock();
	block_data->metaunlock();

	off64_t pos;
	if(-1 == (pos=lseek64(fd, start_point, SEEK_SET)))
	{
		perror("Seek"); 
		_LOG("path %s offset %ld fd %d\n", real_path.c_str(), start_point, fd);
		throw std::runtime_error("Seek File Error"); 
	}
	_DEBUG("write to %s, size=%ld, offset=%ld\n",
			real_path.c_str(), len, start_point);
	while(0 != len && 0 != (ret=write(fd, buf, len)))
	{
		if(-1 == ret)
		{
			if(EINVAL == errno || EAGAIN == errno)
			{
				continue;
			}
			perror("do_recv");
			break;
		}
		buf += ret;
		len -= ret;
	}

	block_data->writing_back=UNSET;
	block_data->dataunlock();

	//sync data to the remote
	//fdatasync(fd);
	_close_remote_file(file_stat, WRITE_FILE);

	return size-len;
}

int IOnode::
_flush_file(extended_IO_task* new_task)
{
	ssize_t file_no=0;
	new_task->pop(file_no);
	try
	{
		file& _file=_files.at(file_no);
		_DEBUG("flush file file_no=%ld, path=%s\n",
				file_no, _file.file_path.c_str());
		//block_info_t &blocks=_file.blocks;
		if(CLEAN == _file.dirty_flag)
		{
			return SUCCESS;
		}
		/*for(block_info_t::iterator it=blocks.begin();
				it != blocks.end();++it)
		{
			block* _block=it->second;
			_block->lock();
			if((DIRTY == _block->dirty_flag && 
					nullptr == _block->write_back_task) ||
					(CLEAN == _block->dirty_flag &&
					 nullptr != _block->write_back_task))
			{
				//_write_to_storage(path, _block);
				_block->write_back_task=
					CBB_remote_task::add_remote_task(
						CBB_REMOTE_WRITE_BACK, _block);
				_DEBUG("add write back\n");
			}
			_block->unlock();
		}*/
		return SUCCESS;
	}
	catch(std::out_of_range& e)
	{
		return FAILURE;
	}
}

int IOnode::
_register_new_client(extended_IO_task* new_task)
{
	char* client_uri=nullptr;
	//getpeername(new_task->get_handle(), reinterpret_cast<sockaddr*>(&addr), &len); 
	_LOG("new client sockfd=%p\n", new_task->get_handle());
	new_task->pop_string(&client_uri);
	_LOG("client uri %s\n", client_uri);

	extended_IO_task* output=init_response_task(new_task);
	output->push_back(SUCCESS);
	output_task_enqueue(output);
	return SUCCESS;
}

int IOnode::
_close_client(extended_IO_task* new_task)
{
	_LOG("close client\n");
	comm_handle_t handle=new_task->get_handle();
	Server::_delete_handle(handle);
	handle->Close();
	return SUCCESS;
}

int IOnode::
_unlink(extended_IO_task* new_task)
{
	ssize_t file_no	=0;
	int	ret	=0;
	_LOG("unlink\n");
	new_task->pop(file_no);
	_LOG("file no=%ld\n", file_no);
	ret=_remove_file(file_no);	
	return ret;
}

inline string IOnode::
_get_real_path(const char* path)const
{
	return _mount_point+std::string(path);
}

inline string IOnode::
_get_real_path(const std::string& path)const
{
	return _mount_point+path;
}

int IOnode::
_truncate_file(extended_IO_task* new_task)
{
	_LOG("truncate file\n");
	off64_t start_point	=0;
	ssize_t fd		=0;
	size_t 	avaliable_size	=0;

	new_task->pop(fd);
	new_task->pop(start_point);
	new_task->pop(avaliable_size);
	_LOG("fd=%ld, start_point=%ld\n", fd, start_point);

	file& _file=_files.at(fd);
	block_info_t::iterator requested_block=_file.blocks.find(start_point);
	requested_block->second->data_size=avaliable_size;
	requested_block++;

	while(end(_file.blocks) != requested_block)
	{ 
		delete requested_block->second;
		_file.blocks.erase(requested_block++);
	}
	return SUCCESS;
}

int IOnode::
_remove_file(ssize_t file_no)
{
	file_t::iterator it=_files.find(file_no);
	_LOG("delete file no %ld\n", file_no);
	if(_files.end() != it &&
		TO_BE_DELETED != it->second.postponed_operation)
	{
		for(block_info_t::iterator block_it=it->second.blocks.begin();
				block_it!=it->second.blocks.end();++block_it)
		{
			block_it->second->postponed_operation=TO_BE_DELETED;
		}
		it->second.postponed_operation=TO_BE_DELETED;
		remove_file_list.push_back(it);
	}
	return SUCCESS;
}

int IOnode::
remote_task_handler(remote_task* new_task)
{
	_DEBUG("start writing back\n");
	writeback_status=ON_GOING;
	int task_id = new_task->get_task_id();
	//file* file_ptr=static_cast<file*>(new_task->get_task_data());
	switch(task_id)
	{
		/*case CBB_REMOTE_CLOSE:
			//close read fd
			if(TO_BE_DELETED != file_ptr->
			if(-1 != file_ptr->read_remote_fd)
			{
				_LOG("Close read file %d\n",
					file_ptr->read_remote_fd);
				close(file_ptr->read_remote_fd);
				file_ptr->read_remote_fd=-1;
			}
			//close write fd
			if(-1 != file_ptr->write_remote_fd)
			{
				_LOG("Close write file %d\n",
					file_ptr->write_remote_fd);
				close(file_ptr->write_remote_fd);
				file_ptr->write_remote_fd=-1;
			}
			break;*/
		case CBB_REMOTE_WRITE_BACK:
			for(auto* block : writeback_queue)
			{
				_DEBUG("writing back file %s\n", 
						block->file_stat->file_path.c_str());
				_DEBUG("block %ld\n", 
						block->start_point);
				writeback(block, "background");
			}break;
	}
	writeback_status=IDLE;
	return SUCCESS;
}

int IOnode::
_heart_beat(extended_IO_task* new_task)
{
	int ret=0;
	new_task->pop(ret);
	return ret;
}

int IOnode::
_node_failure(extended_IO_task* new_task)
{
	comm_handle_t handle=new_task->get_handle();
	_DEBUG("node failure, close handle %p\n", handle);
	handle->Close();

	if(handle->compare_handle(&master_handle))
	{
		_DEBUG("Master failed !!!\n");
	}
	else
	{
		//remove handle if failed node if IOnode
		for(auto& node:IOnode_handle_pool)
		{
			if(handle->compare_handle(&node.second))
			{
				IOnode_handle_pool.erase(node.first);
				break;
			}
		}
	}
	return SUCCESS;
}

int IOnode::
data_sync_parser(data_sync_task* new_task)
{
	int ans=SUCCESS; 
	switch(new_task->task_id)
	{
	case DATA_SYNC_WRITE:
		_sync_write_data(new_task); break; 
	case DATA_SYNC_INIT:
		_sync_init_data(new_task);break;
	}
	return ans; 
}

int IOnode::
_sync_init_data(data_sync_task* new_task)
{
	file* requested_file=static_cast<file*>(new_task->_file);
	_LOG("send sync data file_no = %ld\n", requested_file->file_no);

	//only sync dirty data
	if(DIRTY == requested_file->dirty_flag)
	{
		_send_sync_data(&new_task->handle, requested_file,
				0, 0, MAX_FILE_SIZE);
	}

	return SUCCESS;
}

int IOnode::
_sync_write_data(data_sync_task* new_task)
{
	file* 		requested_file	=static_cast<file*>(new_task->_file);
	comm_handle_t	handle		=&new_task->handle;
	int 		receiver_id	=new_task->receiver_id;
	_DEBUG("send sync data file_no = %ld\n", requested_file->file_no);

#ifdef STRICT_SYNC_DATA
	for(auto& replicas:requested_file->IOnode_pool)
	{
		_send_sync_data(replicas.second, 
				requested_file, 
				new_task->start_point,
				new_task->offset,
				new_task->size);
	}
#endif
	_DEBUG("send response to handle %p\n", handle);
	extended_IO_task* output_task=allocate_data_sync_task();
	output_task->set_handle(handle);
	output_task->set_receiver_id(receiver_id);
#ifdef CCI
	output_task->set_extended_data_size(new_task->size);
	output_task->set_send_buffer(new_task->get_send_buffer());
#endif
	output_task->push_back(SUCCESS);
	data_sync_task_enqueue(output_task);
#ifndef STRICT_SYNC_DATA
	for(auto& replicas:requested_file->IOnode_pool)
	{
		_send_sync_data(replicas.second,
				requested_file, 
				new_task->start_point, 
				new_task->offset, 
				new_task->size);
	}
#endif
#ifdef SYNC_DATA_WITH_REPLY
	for(auto i=requested_file->IOnode_pool.size();
		i!=0; i--)
	{
		_get_sync_response();
	}
#endif
	//new_dirty_page();

	return SUCCESS;
}

int IOnode::
_get_sync_response()
{
	int ret=SUCCESS;

	extended_IO_task* response=get_data_sync_response();
	response->pop(ret);
	_DEBUG("got sync response from %p\n", response->get_handle());

	data_sync_response_dequeue(response);
	return ret;
}

int IOnode::
_send_sync_data(comm_handle_t 	handle,
		file* 		requested_file,
		off64_t 	start_point,
		off64_t		offset,
		ssize_t 	size)
{
	ssize_t ret_size=0;
	_DEBUG("send sync to %p\n", handle);
	extended_IO_task* output_task=allocate_data_sync_task();
	output_task->set_handle(handle);
	output_task->set_receiver_id(0);
	output_task->push_back(DATA_SYNC);
	output_task->push_back(requested_file->file_no);
	output_task->push_back(start_point);
	output_task->push_back(offset);

	auto block_it=requested_file->blocks.find(start_point);
	while(end(requested_file->blocks) != block_it && 0 < size)
	{
		block* requested_block=block_it->second;
		output_task->push_send_buffer(
			static_cast<char*>(requested_block->data),
			requested_block->data_size, requested_block);
		size-=requested_block->data_size;
		ret_size+=requested_block->data_size;
		block_it++;
	}
	output_task->push_back(ret_size);
	_DEBUG("data size=%ld\n", ret_size);
	data_sync_task_enqueue(output_task);
	return SUCCESS;
}

int IOnode::
_get_sync_data(extended_IO_task* new_task)
{
	_LOG("sync data\n");
	ssize_t file_no		=0;
	int 	ret		=SUCCESS;
	off64_t start_point	=0;
	off64_t offset		=0;
	ssize_t size		=0;
	new_task->pop(file_no);
	new_task->pop(start_point);
	new_task->pop(offset);
	new_task->pop(size);
#ifdef SYNC_DATA_WITH_REPLY
	extended_IO_task* output=init_response_task(new_task);
#endif
	try
	{
		file& _file=_files.at(file_no);
		block_info_t &blocks=_file.blocks;
		while(0 != size)
		{
			ssize_t IO_size=MIN(size, static_cast<ssize_t>(BLOCK_SIZE));
			size-=update_block_data(blocks, _file,
				start_point, 0, IO_size, new_task);
			start_point+=IO_size;
		}
#ifdef SYNC_DATA_WITH_REPLY
		output->push_back(SUCCESS);
#endif
		ret=SUCCESS;
	}
	catch(std::out_of_range& e)
	{
#ifdef SYNC_DATA_WITH_REPLY
		output->push_back(FILE_NOT_FOUND);
#endif
		ret=FAILURE;
	}
#ifdef SYNC_DATA_WITH_REPLY
	output_task_enqueue(output);
#endif
	return ret;
}

void IOnode::
configure_dump()
{
	_LOG("under master %s\n", master_uri.c_str());
	_LOG("mount point=%s\n", _mount_point.c_str());
	_LOG("max block %d\n", _MAX_BLOCK_NUMBER);
	_LOG("total memory %ld bytes\n", 
		memory_pool_for_blocks.get_total_memory_size()); //remain available memory; 
}

CBB::CBB_error IOnode::
connection_failure_handler(extended_IO_task* input_task)
{
	communication_queue_t& 	input_queue	=_communication_input_queue.at(input_task->get_id());
	extended_IO_task* 	new_task	=input_queue.allocate_tmp_node();

	//reply with node failure
	new_task->set_error(CONNECTION_SETUP_FAILED);
	input_queue.task_enqueue_signal_notification();
	return SUCCESS;
}

int IOnode::
update_access_order(block* requested_block, bool dirty)
{
	if(nullptr != requested_block->writeback_page)
	{
		access(requested_block->writeback_page, dirty);
	}
	else
	{
		requested_block->writeback_page=access(requested_block, dirty);
		_DEBUG("write back page %p\n", requested_block->writeback_page);
	}
	return SUCCESS;
}

size_t IOnode::
writeback(block* requested_block, const char* mode)
{
	//_write_to_storage(path, _block);
	if(DIRTY == requested_block->dirty_flag)
	{
		size_t ret=0;
		_DEBUG("writing back %p\n", requested_block);
		ret=_write_to_storage(requested_block, mode);
		_DEBUG("finishing writing %p\n", requested_block);
		return ret;
	}
	else
	{
		return 0;
	}
}

int IOnode::
interval_process()
{
	remove_files();
	add_write_back();
	return SUCCESS;
}

int IOnode::
add_write_back()
{
	if(IDLE == writeback_status) //&& 
		//have_dirty_page())
	{
#ifdef ASYNC
		Common::CBB_swap<block>::check_writeback_page(writeback_queue);
		if(0 != prepare_writeback_page(&writeback_queue, WRITEBACK_COUNTS))
		{
			CBB_remote_task::add_remote_task(
					CBB_REMOTE_WRITE_BACK, nullptr);
			_DEBUG("add write back\n");
			//clear_dirty_page();
		}
#endif
	}
	return SUCCESS;
}

block* IOnode::
create_new_block(off64_t start_point,
		size_t	 data_size,
		bool 	 dirty_flag,
		bool 	 valid,
		file* 	 file_stat)
{
	if(VALID == valid)
	{
		if(!has_memory_left(BLOCK_SIZE))
		{
			free_memory(BLOCK_SIZE);
		}
	}

	return new block(start_point, data_size, dirty_flag, 
			valid, file_stat, memory_pool_for_blocks);
}

size_t IOnode::
free_memory(size_t size)
{
	_DEBUG("free memory for new blocks\n");
	return CBB_swap<block>::swap_out(size);
}

size_t IOnode::
allocate_memory_for_block(block* requested_block)
{
	size_t allocate_size=requested_block->block_size;

	if(!has_memory_left(allocate_size))
	{
		free_memory(allocate_size);
	}

	return requested_block->allocate_memory();
}

size_t IOnode::
_set_memory_limit(const char* limit_string)
throw(std::runtime_error)
{
	static std::regex regex_text("^(\\d+)( ?(KB|MB|GB|KiB|MiB|GiB))?$");
	std::cmatch matches;
	size_t total_memory=0;

	if(regex_match(limit_string, matches, regex_text))
	{
		total_memory=atoi(matches[0].str().c_str());
		if(1 < matches.size())
		{
			const char* suffix=matches[2].str().c_str();
			if(0 == strcmp(suffix, "KB"))
			{
				total_memory*=KB;
			}
			else if(0 == strcmp(suffix, "MB"))
			{
				total_memory*=MB;
			}
			else if(0 == strcmp(suffix, "GB"))
			{
				total_memory*=GB;
			}
			else if(0 == strcmp(suffix, "KiB"))
			{
				total_memory*=KiB;
			}
			else if(0 == strcmp(suffix, "MiB"))
			{
				total_memory*=MiB;
			}
			else if(0 == strcmp(suffix, "GiB"))
			{
				total_memory*=GiB;
			}
		}
		_LOG("total memory %ld\n", total_memory);
		return total_memory;
	}
	else
	{
		throw runtime_error(
			"please set HUFS_IONODE_MEMORY_LIMIT to x {KB, MB, GB, KiB, MiB, GiB}\n");
	}
}

int IOnode::
remove_files()
{
	if(IDLE == writeback_status && 
		0 != remove_file_list.size())
	{
		for(auto file:remove_file_list)
		{
			_close_remote_file(&file->second, DELETE_FILE);
			_files.erase(file);
		}
		remove_file_list.clear();
	}
	return SUCCESS;
}

size_t IOnode::
free_data(block* data)
{
	_DEBUG("free memory of %s start point %ld\n",
			data->file_stat->file_path.c_str(),
			data->start_point);
	data->metalock();

	bool writing_back=UNSET;
	struct timeval st, et;
	//wait for ongoing write back to finish 
	_DEBUG("testing block %p\n", data);
	if(SET == data->writing_back)
	{
		_LOG("write back on going\n");
		writing_back=true;
		gettimeofday(&st, nullptr);
	}

	while(SET == data->writing_back);
	_DEBUG("start to free block %p\n", data);

	if(writing_back)
	{
		//tmp code here
		gettimeofday(&et, nullptr);
		_LOG("waiting write back time %f of %p\n", TIME(st, et), data);
	}

	data->dataWrlock();
	size_t ret=data->free_memory();
	data->dataunlock();
	data->swapout_flag=SWAPPED_OUT;
	data->metaunlock();
	return ret;
}
