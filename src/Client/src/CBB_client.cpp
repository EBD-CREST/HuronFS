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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <map>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <limits.h>
#include <errno.h>
//#include <functional>

#include <execinfo.h>

#include "CBB_client.h"
#include "CBB_internal.h"
//#include "Communication.h"

using namespace CBB::Common;
using namespace CBB::Client;
using namespace std;

const char* CBB_client::CLIENT_MOUNT_POINT="HUFS_CLIENT_MOUNT_POINT";
const char* CBB_client::CLIENT_USING_IONODE_CACHE="HUFS_USING_IONODE_CACHE";
const char* CBB_client::MASTER_IP_LIST="HUFS_MASTER_IP_LIST";
const char *mount_point=nullptr;

/*#define CHECK_INIT()                                                      \
  do                                                                \
  {                                                                 \
  if(!_initial)                                             \
  {                                                         \
  fprintf(stderr, "initialization unfinished\n");   \
  errno = EAGAIN;                                   \
  return -1;                                        \
  }                                                         \
  }while(0)*/

//tmp solution
#define CHECK_INIT()                                              \
	do                                                        \
{                                                                 \
	if(!_thread_started)                                      \
	{                                                         \
		printf("init\n");				  \
		start_threads();				  \
	}                                                         \
}while(0)

CBB_client::CBB_client()try:
	Client(CLIENT_QUEUE_NUM),
	_fid_now(0),
	_file_list(_file_list_t()),
	_opened_file(_file_t(MAX_FILE)),
	_path_file_meta_map(),
	_client_addr(sockaddr_in()),
	_initial(true),
	_thread_started(false),
	master_handle_list(),
	my_uri(),
	USING_IONODE_CACHE(false)
	//files(MAX_DIR_FILE_SIZE)
	//IOnode_fd_map()
{
	_DEBUG("start initalizing\n");
	const char* use_IOnode_cache=nullptr;

	if(nullptr == (mount_point=getenv(CLIENT_MOUNT_POINT)))
	{
		fprintf(stderr, "please set mount point\n");
		_initial=false;
		return;
	}

	if((nullptr != (use_IOnode_cache=getenv(CLIENT_USING_IONODE_CACHE))))
	{
		if(0 == strncmp(use_IOnode_cache, "true", strlen("true")))
		{
			_LOG("using IOnode cache\n");
			USING_IONODE_CACHE=true;
		}
		else
		{
			if(0 == strncmp(use_IOnode_cache, "false", strlen("false")))
			{
				_LOG("disable IOnode cache\n");
			}
			else
			{
				_LOG("please set %s to \"true\" or \"false\"\n",
					CLIENT_USING_IONODE_CACHE);
				_initial=false;
				return;
			}
		}
	}
	//_DEBUG("%s\n", mount_point);
	memset(&_client_addr, 0, sizeof(_client_addr));
	for(int i=0;i<MAX_FILE; ++i)
	{
		_opened_file[i]=false;
	}
}
//try catch of the initialization list
catch(exception &e)
{
	_LOG("error message %s\n", e.what());
	_initial=false;
}

void CBB_client::
start_threads()
{
	_client_addr.sin_family = AF_INET;
	_client_addr.sin_port = htons(0);
	_client_addr.sin_addr.s_addr = htons(INADDR_ANY);
	Client::start_client();
	if(-1 == _register_to_master())
	{
		return;
	}
	_DEBUG("initialization finished\n");
	_thread_started=true;
}

int CBB_client::
_register_to_master()
{
	int ret=0;
	const char* master_ip_list=getenv(MASTER_IP_LIST);
	char master_ip[IP_STRING_LEN];
	if(nullptr == master_ip_list)
	{
		fprintf(stderr, "please set master ip\n");
		return -1;
	}

	int counter=0;
	while(nullptr != (master_ip_list =parse_master_config_ip(master_ip_list, master_ip)))
	{
		extended_IO_task* query=allocate_new_query(master_ip, MASTER_PORT);
		query->push_back(NEW_CLIENT);
		send_query(query);

		try
		{
			extended_IO_task* response=get_query_response(query);
			response->pop(ret);
			master_handle_list.push_back(SCBB(counter++, response->get_handle()));

			response_dequeue(response);
		}
		catch(std::runtime_error &e)
		{
			_LOG("failed to connect to Master %s\n", master_ip);
			return -1;
		}
	}
	return ret;
}

CBB_client::
~CBB_client()
{
	for(_file_list_t::iterator it=_file_list.begin();
			it!=_file_list.end();++it)
	{
		int ret=0;
		extended_IO_task* query=
			allocate_new_query(it->second.file_meta_p->get_master_handle());
		query->push_back(CLOSE_FILE);
		query->push_back(it->second.get_remote_file_no());
		send_query(query);

		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		response_dequeue(response);
		release_communication_queue(query);
	}
	for(master_list_t::iterator it=master_handle_list.begin();
			it != master_handle_list.end();++it)
	{
		extended_IO_task* query=allocate_new_query(it->get_master_handle());
		query->push_back(CLOSE_CLIENT);
		send_query(query);
		release_communication_queue(query);

		IOnode_fd_map_t & IOnode_list=it->get_IOnode_list();
		for(IOnode_fd_map_t::iterator IOnode_it=begin(IOnode_list);
				IOnode_it != end(IOnode_list);++IOnode_it)
		{
			extended_IO_task* query=allocate_new_query(&IOnode_it->second);
			query->push_back(CLOSE_CLIENT);
			send_query(query);
			release_communication_queue(query);
		}

	}
	/*for(IOnode_fd_map_t::iterator it=IOnode_fd_map.begin();
	  it!=IOnode_fd_map.end();++it)
	  {
	  extended_IO_task* query=allocate_new_query(it->second);
	  query->push_back(CLOSE_CLIENT);
	  send_query(query);
	  release_communication_queue(query);
	  }*/
}

int CBB_client::
_parse_blocks_from_master(
		extended_IO_task* response,
		SCBB*		  corresponding_SCBB,
		off64_t 	  start_point,
		size_t& 	  size,
		_block_list_t&    block,
		_block_list_t&	  block_cache,
		_node_pool_t&     node_pool,
		IOnode_list_t&	  IOnode_cache)
{
	char* 		uri		=nullptr;
	int 		count		=0;
	ssize_t 	node_id		=0;
	size_t 		block_size	=0;

	response->pop(count);
	for(int i=0;i<count;++i)
	{
		response->pop(node_id);
		response->pop_string(&uri);
		comm_handle_t	IOnode_handle=
			_get_IOnode_handle(corresponding_SCBB, node_id, uri);
		node_pool.insert(make_pair(node_id, IOnode_handle));
		if(std::find(begin(IOnode_cache), 
			end(IOnode_cache), 
			node_id) == end(IOnode_cache))
		{
			IOnode_cache.push_back(node_id);
		}
	}
	response->pop(count);
	for(int i=0;i<count;++i)
	{
		response->pop(start_point);
		//response->pop(node_id);
		response->pop(block_size);
		block.insert(make_pair(start_point, block_size));

		//always insert, will return false when the block is 
		//already in the cache, but ignore the return value
		block_cache.insert(make_pair(start_point, block_size));
		/*_block_list_t::iterator it;
		if(std::end(block_cache) != 
				(it=std::find(std::begin(block_cache),
				std::end(block_cache), start_point)))
		{
			block_cache.insert(make_pair(start_point, block_size));
		}*/
			
	}
	return SUCCESS;
}


int CBB_client::_get_fid()
{
	for(int i=_fid_now; i<MAX_FILE; ++i)
	{
		if(!_opened_file[i])
		{
			_opened_file[i]=true;
			_fid_now=i;
			return i;
		}
	}
	for(int i=0;i<_fid_now; ++i)
	{
		if(!_opened_file[i])
		{
			_opened_file[i]=true;
			_fid_now=i;
			return i;
		}
	}
	return -1;
}

file_meta* CBB_client::
remote_open(const char* 	path,
	   int 	  		flag,
	   mode_t 	  	mode)
{
	int 		  master_number	=_get_master_number_from_path(path);
	comm_handle_t	  master_handle	=_get_master_handle_from_master_number(master_number);
	extended_IO_task* response	=nullptr;
	file_meta* 	  file_meta_p	=nullptr;
	int 		  ret		=master_number;

	_DEBUG("connect to master\n");
	do{
		master_number = ret;
		extended_IO_task* query=allocate_new_query(master_handle);
		query->push_back(OPEN_FILE); 
		query->push_back_string(path);

		if(flag & O_CREAT)
		{
			query->push_back(flag); 
		}
		query->push_back(mode);
		send_query(query);
		response=get_query_response(query);
		master_handle = _get_master_handle_from_master_number(ret);
	}while(master_number != ret && response_dequeue(response));
	response->pop(ret);
	if(SUCCESS == ret)
	{
		_DEBUG("open finished\n");
		if(nullptr == file_meta_p)
		{
			file_meta_p=
				_create_new_file(response,
						_get_scbb_from_master_number(master_number));

			_path_file_meta_map_t::iterator it=
				_path_file_meta_map.insert(
						make_pair(std::string(path),
							file_meta_p)).first;
			file_meta_p->it=it;
		}
		else
		{
			response->pop(file_meta_p->remote_file_no);
			response->pop(file_meta_p->block_size);
			Recv_attr(response, &file_meta_p->file_stat);
		}
	}
	else
	{
		errno=-ret;
		_DEBUG("server return error!");
	}
	response_dequeue(response);

	return  file_meta_p;
}

int CBB_client::
_open(const char* path,
      int 	  flag,
      mode_t 	  mode)
throw(std::runtime_error)
{
	CHECK_INIT();
	file_meta* 	  file_meta_p	=nullptr;
	string 	  	  string_path	=string(path);
	int 		  fid		=0;

	if(-1 == (fid=_get_fid()))
	{
		errno = EMFILE;
		_DEBUG("error!");
		return -1; 
	}
	try
	{
		file_meta_p=_path_file_meta_map.at(string_path);
	}
	catch(std::out_of_range& e)
	{
		//forwarding to remote open
		file_meta_p=remote_open(path, flag, mode);
	}
	int fd=_fid_to_fd(fid);
	opened_file_info& file=
		_file_list.insert(make_pair(fid,
			opened_file_info(fd, 
			flag, file_meta_p))).first->second;
	file_meta_p->opened_fd.insert(fd);
	if(flag & O_APPEND)
	{
		file.current_point=file_meta_p->file_stat.st_size;
	}
	_DEBUG("file no =%ld, fd = %d\n", file_meta_p->remote_file_no, fd);

	errno=0;
	return fd;
}

file_meta* CBB_client::
_create_new_file(extended_IO_task* response,
		SCBB* 		   corresponding_SCBB)
{
	ssize_t 	remote_file_no	=0;
	size_t 		block_size	=0;
	struct stat 	file_stat;

	response->pop(remote_file_no);
	response->pop(block_size);
	Recv_attr(response, &file_stat);

	file_meta* file_meta_p=
		new file_meta(remote_file_no, 
			block_size, &file_stat, corresponding_SCBB);

	if(nullptr != file_meta_p)
	{
		return file_meta_p;
	}
	else
	{
		perror("malloc");
		_DEBUG("malloc error");
		exit(EXIT_FAILURE);
	}
}

ssize_t CBB_client::
_read_from_IOnode(
		file_meta* 	file_meta,
		const _block_list_t& 	blocks,
		const _node_pool_t& 	node_pool,
		char* 		buffer,
		off64_t		offset,
		size_t 		size)
{
	ssize_t 	   ans		=0;
	int 		   ret		=0;
	off64_t 	   current_point=offset;
	size_t 		   read_size	=size;
	off64_t 	   start_point	=begin(blocks)->first;
	extended_IO_task*  response	=nullptr;
	extended_IO_task*  query	=nullptr;
	comm_handle_t	   IOnode_handle=begin(node_pool)->second;

	while(0 < read_size)
	{
		//profiling
		//start_recording();

		off64_t offset = current_point-start_point;
		size_t IO_size = MAX_TRANSFER_SIZE > read_size? read_size : MAX_TRANSFER_SIZE;

		do
		{
			query=allocate_new_query(IOnode_handle);
			query->setup_large_transfer(RMA_READ);
			query->push_back(READ_FILE);
			query->push_back(file_meta->get_remote_file_no());
			query->push_back(start_point);
			query->push_back(offset);
			query->push_back(IO_size);
#ifdef CCI
			query->push_send_buffer(buffer, IO_size);
#endif
			send_query(query);

			response=get_query_response(query);
			response->pop(ret);
		}
		while(FILE_NOT_FOUND == ret && 
			0 == response_dequeue(response));
		if(SUCCESS == ret)
		{
			ssize_t ret_size=response->get_received_data_all(buffer);

			_DEBUG("IO_size=%lu, start_point=%ld, remote_file_no=%ld, ret_size=%ld\n",
				IO_size, start_point,
				file_meta->remote_file_no, ret_size);
			if(-1 == ret_size)
			{
				response_dequeue(response);
				return ans;
			}
			else
			{
				_DEBUG("read size=%ld current point=%ld\n",
					ret_size, current_point);
				ans+=ret_size;
				//file.current_point += ret_size;
				current_point += ret_size;
				start_point=find_start_point(blocks, start_point, ret_size);
				read_size -= ret_size;
				buffer+=ret_size;
				cleanup_after_large_transfer(query);
			}
		}
		//*buffer=0;
		response_dequeue(response);
		//profiling
		//end_recording();
	}
	return ans;
}

ssize_t CBB_client::
_write_to_IOnode(
		file_meta* 	file_meta,
		const _block_list_t& 	blocks,
		const _node_pool_t& 	node_pool,
		char* 		buffer,
		off64_t		offset,
		size_t 		size)
{
	ssize_t 		ans		=0;
	int 			ret		=0;
	off64_t 		current_point	=offset;
	size_t 			write_size	=size;
	off64_t 		start_point	=begin(blocks)->first;
	comm_handle_t		IOnode_handle	=begin(node_pool)->second;
	extended_IO_task* 	response	=nullptr;
	extended_IO_task* 	query		=nullptr;

	while(0 < write_size)
	{
		//profiling
		//start_recording();

		off64_t offset = current_point-start_point;
		size_t IO_size = MAX_TRANSFER_SIZE > write_size? write_size: MAX_TRANSFER_SIZE;

		_DEBUG("IO_size=%lu, start_point=%ld, remote_file_no=%ld\n",
				IO_size, start_point, file_meta->get_remote_file_no());
		do
		{
			query=allocate_new_query(IOnode_handle);
			query->setup_large_transfer(RMA_WRITE);
			query->push_back(WRITE_FILE);
			query->push_back(file_meta->get_remote_file_no());
			query->push_back(start_point);
			query->push_back(offset);
			query->push_back(IO_size);
			query->push_send_buffer((char*)buffer, IO_size);
			send_query(query);

			response=get_query_response(query);
			response->pop(ret);
		}
		while(FILE_NOT_FOUND == ret &&
				0 == response_dequeue(response));
		if(SUCCESS == ret)
		{
			ssize_t ret_size=IO_size;
			if(-1 == ret_size)
			{
				response_dequeue(response);
				return ans;
			}
			else
			{
				_DEBUG("write size=%ld current point=%ld\n",
						ret_size, current_point);
				ans+=ret_size;
				//file.current_point+=ret_size;
				current_point += ret_size;
				start_point=find_start_point(blocks,
						start_point, ret_size);
				buffer+=ret_size;
				write_size -= ret_size;
				cleanup_after_large_transfer(query);
			}
		}
		response_dequeue(response);
		//end_recording();
	}	
	_write_update_file_size(file_meta, offset, ans);
	return ans;
}

comm_handle_t CBB_client::
_get_IOnode_handle(
		SCBB	 	*corresponding_SCBB,
		ssize_t 	IOnode_id,
		const char*	ip)
{
	//start_recording();
	try
	{
		//end_recording();
		return corresponding_SCBB->get_IOnode_fd(IOnode_id);
	}
	catch(std::out_of_range& e)
	{
		int ret=FAILURE;
		comm_handle_t IOnode_handle=nullptr;

		extended_IO_task* query=allocate_new_query(ip, IONODE_PORT);
		query->push_back(NEW_CLIENT);
		send_query(query);

		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		if(SUCCESS == ret)
		{
			IOnode_handle=
				corresponding_SCBB->insert_IOnode(
					IOnode_id, response->get_handle());
			//end_recording();
		}
		response_dequeue(response);
		return IOnode_handle;
	}
}

int CBB_client::
_get_blocks_from_master(
		file_meta* 	file_meta_p,
		off64_t		start_point,
		size_t 	  	size, 
		_block_list_t&	blocks,
		_node_pool_t&	node_pool,
		int 		mode)
{
	int ret=0;
	int fd=file_meta_p->get_remote_file_no();
	_DEBUG("connect to master to get blocks\n");
	comm_handle_t master_handle=file_meta_p->get_master_handle();
	extended_IO_task* query=allocate_new_query(master_handle);
	query->push_back(mode);
	query->push_back(file_meta_p->remote_file_no);
	if(WRITE_FILE == mode)
	{
		Send_attr(query, &file_meta_p->file_stat);
	}
	query->push_back(start_point);
	query->push_back(size);
	send_query(query);

	extended_IO_task* response=get_query_response(query);
	response->pop(ret);
	if(SUCCESS == ret)
	{
		_update_file_size_from_master(response, file_meta_p);
		_parse_blocks_from_master(response, file_meta_p->corresponding_SCBB, 
				start_point, size, blocks, file_meta_p->block_list, 
				node_pool, file_meta_p->IOnode_list_cache);
		response_dequeue(response);
	}
	else
	{
		errno=-ret;
		ret=-1;
		response_dequeue(response);
	}
	return ret;
}

int CBB_client::
_get_blocks_from_cache(
		file_meta*	file_meta_p,
		off64_t		start_point,
		size_t 	  	size,
		_block_list_t&	block_list,
		_node_pool_t&	node_pool)
{
	SCBB*  corresponding_SCBB=file_meta_p->corresponding_SCBB;
	_DEBUG("getting blocks from cache\n");

	if(static_cast<ssize_t> (size + start_point)
		> file_meta_p->get_file_stat().st_size)
	{
		//append new blocks, go to master
		return FAILURE;
	}
	for(IOnode_list_t::const_iterator it = begin(file_meta_p->IOnode_list_cache);
			it != end(file_meta_p->IOnode_list_cache); ++it)
	{
		node_pool.insert(make_pair(*it, 
				&corresponding_SCBB->get_IOnode_handle(*it)));
	}
	_block_list_t& blocks=file_meta_p->block_list;

	//_block_list_t::const_iterator it=find_block_by_start_point(
	//		blocks, get_block_start_point(file.current_point));
	_block_list_t::const_iterator it=
			blocks.find(get_block_start_point(start_point));

	for(;0 < size && it != std::end(blocks); ++it)
	{
		size_t IO_size=MAX(it->second, size);
		block_list.insert(make_pair(it->first, 
					IO_size));
		size-=IO_size;
	}

	if(0 < size)
	{
		//still some blocks left, which are not buffered locally
		//go to master
		return FAILURE;
	}
	else
	{
		return SUCCESS;
	}
}

ssize_t CBB_client::
remote_IO(file_meta*	file_meta_p,
	   void*     	buffer,
	   off64_t	start_point,
	   size_t     	size,
	   int 		mode)
{

	ssize_t IO_size=0;
	_block_list_t blocks;
	_node_pool_t node_pool;
	int ret=SUCCESS;

	if((!_using_IOnode_cache()) || 
			SUCCESS != (ret=_get_blocks_from_cache(file_meta_p,
					start_point, size, blocks, node_pool)))
	{
		//if using cache and failed
		//if not using cache
		ret=_get_blocks_from_master(file_meta_p, start_point, size,
				blocks, node_pool, mode);
	}

	if(SUCCESS == ret)
	{
		if(WRITE_FILE == mode)
		{
			IO_size+=_write_to_IOnode(file_meta_p, blocks, node_pool,
					static_cast<char *>(buffer), start_point, size);
		}
		else if(READ_FILE == mode)
		{
			IO_size+=_read_from_IOnode(file_meta_p, blocks, node_pool,
				static_cast<char *>(buffer), start_point, size);
		}
	}
	else
	{
		errno=EINVAL;
	}
	return IO_size;
}

ssize_t CBB_client::
_read(int 	fd,
      void* 	buffer,
      size_t 	size)
throw(std::runtime_error)
{
	ssize_t read_size=0;
	CHECK_INIT();
	if(0 == size)
	{
		return size;
	}
	int fid=_fd_to_fid(fd);
	try
	{
		opened_file_info& file	  =_file_list.at(fid);

		if(size + file.current_point > 
			static_cast<size_t>(file.get_file_metadata().st_size))
		{
			if(file.get_file_metadata().st_size>file.current_point)
			{
				size=file.get_file_metadata().st_size-file.current_point;
			}
			else
			{
				return 0;
			}
		}

		//forwarding to basic_read for remote reading
		read_size=remote_IO(file.get_meta_pointer(), 
				buffer, file.current_point, size, READ_FILE);
		file.current_point+=read_size;

	}
	catch(std::out_of_range &e)
	{
		errno = EBADFD;
		read_size=-1;
	}
	return read_size;
}

ssize_t CBB_client::
_write(	int 		fd, 
	const void* 	buffer,
	size_t 		size)
throw(std::runtime_error)
{
	int write_size	=0;
	int fid		=_fd_to_fid(fd);
	CHECK_INIT();

	if(0 == size)
	{
		return size;
	}
	try
	{
		opened_file_info& file	 	=_file_list.at(fid);
		//off64_t&	  current_point	=file.current_point;
		_block_list_t 	  blocks;
		_node_pool_t 	  node_pool;

		//_write_update_file_size(file, size);
		_update_access_time(fd);

		//bad hack
		write_size += remote_IO(file.get_meta_pointer(), 
				const_cast<void*>(buffer), file.current_point, size, WRITE_FILE);
		file.current_point+=write_size;
	}
	catch(std::out_of_range &e)
	{
		errno = EBADFD;
		write_size=-1;
	}
	return write_size;

}

size_t CBB_client::
_update_file_size_from_master(
		extended_IO_task* response,
		file_meta*	  file_meta_p)
		//opened_file_info& file)
{
	size_t file_size;
	response->pop(file_size);
	if(file_size > file_meta_p->get_file_stat().st_size)
	{
		file_meta_p->file_stat.st_size = file_size;
	}
	return file_size;
}
int CBB_client::
_write_update_file_size(
		file_meta* file_meta_p,
		off64_t	   current_point,
		size_t 	   size)
{
	if(current_point + size > (size_t)file_meta_p->file_stat.st_size)
	{
		file_meta_p->file_stat.st_size=current_point + size;
	}
	return SUCCESS;
}

int CBB_client::
_update_file_size(int fd, size_t size)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	try
	{
		opened_file_info& file=_file_list.at(fid);
		if((size_t)file.file_meta_p->file_stat.st_size < size)
		{
			file.file_meta_p->file_stat.st_size = size;
		}
		return 0;
	}
	catch(std::out_of_range&e)
	{
		errno=EBADFD;
		return -1;
	}
}

int CBB_client::
_update_access_time(int fd)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	try
	{
		opened_file_info& file=_file_list.at(fid);
		time_t new_time=time(nullptr);
		file.get_file_metadata().st_mtime=new_time;
		file.get_file_metadata().st_atime=file.get_file_metadata().st_mtime;
		return 0;
	}
	catch(std::out_of_range&e)
	{
		errno=EBADFD;
		return -1;
	}
}

int CBB_client::
_close(int fd)
throw(std::runtime_error)
{
	int ret=0;
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	comm_handle_t master_handle=_get_master_handle_from_fd(fid);
	try
	{
		opened_file_info& file	=_file_list.at(fid);
		extended_IO_task* query	=allocate_new_query(master_handle);
		query->push_back(CLOSE_FILE);
		query->push_back(file.get_remote_file_no());
		send_query(query);

		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		response_dequeue(response);
		if(SUCCESS == ret)
		{
			_opened_file[fid]=false;
			if(1 == file.file_meta_p->open_count)
			{
				_path_file_meta_map.erase(file.file_meta_p->it);
			}
			_file_list.erase(fid);

			return 0;
		}
		else
		{
			errno=-ret;
			return -1;
		}
	}
	catch(std::out_of_range &e)
	{
		errno=EBADFD;
		return -1;
	}
}

int CBB_client::
_flush(int fd)
throw(std::runtime_error)
{
	int ret=0;
	int fid=_fd_to_fid(fd);
	comm_handle_t master_handle=_get_master_handle_from_fd(fid);
	CHECK_INIT();
	try
	{
		opened_file_info& file =_file_list.at(fid);
		extended_IO_task* query=allocate_new_query(master_handle);
		query->push_back(FLUSH_FILE);
		query->push_back(file.get_remote_file_no());
		send_query(query);

		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		response_dequeue(response);
		if(SUCCESS == ret)
		{
			return 0;
		}
		else
		{
			errno=-ret;
			return -1;
		}
	}
	catch(std::out_of_range& e)
	{
		errno=EBADFD;
		return -1;
	}
}

off64_t CBB_client::
_lseek(	int 	fd,
	off64_t offset,
	int 	whence)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	try
	{
		opened_file_info & file=_file_list.at(fid);
		switch(whence)
		{
			case SEEK_SET:
				file.current_point=offset;break;
			case SEEK_CUR:
				file.current_point+=offset;break;
			case SEEK_END:
				file.current_point=file.file_meta_p->file_stat.st_size+offset;break;
		}
		return offset;
	}
	catch(std::out_of_range &e)
	{
		errno=EBADF;
		return -1;
	}
}

int CBB_client::
remote_getattr(	const char* 	path,
	 	struct stat* 	fstat)
{
	int master_number=_get_master_number_from_path(path);
	comm_handle_t master_handle=_get_master_handle_from_master_number(master_number);
	int ret=master_number;
	extended_IO_task* response=nullptr;
	_DEBUG("connect to master\n");

	do{
		//problem
		master_number=ret;
		extended_IO_task* query=allocate_new_query(master_handle);
		query->push_back(GET_ATTR); 
		query->push_back_string(path);

		send_query(query);

		response=get_query_response(query);
		response->pop(ret);
		master_handle = _get_master_handle_from_master_number(ret);
	}while(master_number != ret && response_dequeue(response));
	//end_recording();
	response->pop(ret);
	if(SUCCESS == ret)
	{
		_DEBUG("SUCCESS\n");
		Recv_attr(response, fstat);
	}
	else
	{
		errno=-ret;
	}
	response_dequeue(response);
	return ret;
}

int CBB_client::
_getattr(const char* path,
	 struct stat* fstat)
throw(std::runtime_error)
{
	CHECK_INIT();

	//start_recording();
	if(SUCCESS == _get_local_attr(path, fstat))
	{
		_DEBUG("use local stat\n");
		return 0;
	}
	else
	{
		//forwarding
		return remote_getattr(path, fstat);
	}
}

int CBB_client::
_readdir(const char * path, dir_t& dir)
throw(std::runtime_error)

{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();

	//putting files in class to reduce the latency
	string_buf files(MAX_DIR_FILE_SIZE);

	for(master_list_t::const_iterator it=master_handle_list.begin();
			it != master_handle_list.end(); ++it)
	{
		comm_handle_t master_handle=&it->master_handle;
		extended_IO_task* query=allocate_new_query(master_handle);
		query->push_back(READ_DIR); 
#ifdef CCI
		query->push_send_buffer((char*)files.get_buffer(), 
				MAX_DIR_FILE_SIZE);
		query->setup_large_transfer(RMA_DIR);
#endif
		query->push_back_string(path);
		send_query(query);

		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		if(SUCCESS == ret)
		{
			_DEBUG("success\n");
			int count=0;
			char *path=nullptr;
			response->pop(count);
#ifdef TCP
			response->get_received_data_all(files.get_buffer());
#endif
			for(int i=0; i<count; ++i)
			{
				files.pop_string(&path);
				dir.insert(std::string(path));
			}
#ifdef CCI
			cleanup_after_large_transfer(query);
#endif

		}
		else
		{
			errno=-ret;
			response_dequeue(response);
			return -errno;
		}
		response_dequeue(response);
	}
	return 0;
}

int CBB_client::
_rmdir(const char * path)
throw(std::runtime_error)

{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	comm_handle_t master_handle=_get_master_handle_from_path(path);

	extended_IO_task* query=allocate_new_query(master_handle);
	query->push_back(RM_DIR); 
	query->push_back_string(path);
	send_query(query);

	extended_IO_task* response=get_query_response(query);
	response->pop(ret);
	response_dequeue(response);
	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		errno=-ret;
		return -errno;
	}
}

int CBB_client::
_unlink(const char * path)
throw(std::runtime_error)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	comm_handle_t master_handle=_get_master_handle_from_path(path);

	extended_IO_task* query=allocate_new_query(master_handle);
	query->push_back(UNLINK); 
	query->push_back_string(path);
	send_query(query);

	extended_IO_task* response=get_query_response(query);
	response->pop(ret);
	response_dequeue(response);

	if(SUCCESS == ret)
	{
		_close_local_opened_file(path);
		return 0;
	}
	else
	{
		errno=-ret;
		return -errno;
	}
}

int CBB_client::
_close_local_opened_file(const char* path)
{
	try
	{
		std::string string_path(path);
		file_meta* file_meta_p=_path_file_meta_map.at(string_path);
		int count=file_meta_p->open_count;
		for(_opened_fd_t::iterator it=file_meta_p->opened_fd.begin();
				it!=file_meta_p->opened_fd.end();++it)
		{
			_close(*it);
			--count;
		}
		if(count)
		{
			delete file_meta_p;
		}
		_path_file_meta_map.erase(string_path);
		return 0;
	}
	catch(std::out_of_range &e)
	{
		return -1;
	}
}

int CBB_client::
remote_access(
		const char* 	path,
		int 		mode)
{
	int master_number=_get_master_number_from_path(path);
	comm_handle_t master_handle=_get_master_handle_from_master_number(master_number);
	int ret=master_number;
	extended_IO_task* response=nullptr;

	do{
		master_number=ret;
		extended_IO_task* query=allocate_new_query(master_handle);
		query->push_back(ACCESS); 
		query->push_back_string(path);
		query->push_back(mode);
		send_query(query);

		response=get_query_response(query);
		response->pop(ret);
		master_handle = _get_master_handle_from_master_number(ret);
	}while(master_number != ret && response_dequeue(response));
	response->pop(ret);
	response_dequeue(response);
	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		errno=-ret;
		return -errno;
	}
}

int CBB_client::
_access(const char* 	path,
	int 		mode)
throw(std::runtime_error)
{
	_DEBUG("connect to master\n");
	CHECK_INIT();
	std::string string_path(path);

	auto file_meta=_path_file_meta_map.find(string_path);
	if(end(_path_file_meta_map) != file_meta)
	{
		return 0;
	}
	else
	{
		return remote_access(path, mode);
	}
}

int CBB_client::
remote_stat(	const char*  path,
		struct stat* buf)
{
	CHECK_INIT();
	memset(buf, 0, sizeof(struct stat));
	int master_number=_get_master_number_from_path(path);
	comm_handle_t master_handle=_get_master_handle_from_master_number(master_number);
	int ret=master_number;
	extended_IO_task* response=nullptr;

	do{
		master_number=ret;
		extended_IO_task* query=allocate_new_query(master_handle);
		query->push_back(GET_FILE_META);
		query->push_back_string(path);
		send_query(query);

		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		master_handle = _get_master_handle_from_master_number(ret);
	}while(master_number != ret && response_dequeue(response));
	response->pop(ret);
	response_dequeue(response);
	if(SUCCESS == ret)
	{
		Recv_attr(response, buf);
		return 0;
	}
	else
	{
		return -1;
	}
}

int CBB_client::
_rename(const char* old_name,
	const char* new_name)
throw(std::runtime_error)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int new_master_number=_get_master_number_from_path(new_name);
	int old_master_number=_get_master_number_from_path(old_name);
	comm_handle_t old_master_handle=
		_get_master_handle_from_master_number(old_master_number);
	comm_handle_t new_master_handle=
		_get_master_handle_from_master_number(new_master_number);

	_path_file_meta_map_t::iterator it=_path_file_meta_map.find(old_name);
	extended_IO_task* response=nullptr;

	//if opened locally, rename local file
	if(_path_file_meta_map.end() != it)
	{

		_path_file_meta_map_t::iterator new_it=
			_path_file_meta_map.insert(make_pair(new_name,
						it->second)).first;
		_path_file_meta_map.erase(it);
		new_it->second->it=new_it;
	}

	//send rename to old master
	extended_IO_task* query=allocate_new_query(old_master_handle);
	query->push_back(RENAME); 
	query->push_back_string(old_name);
	query->push_back_string(new_name);

	if(new_master_number == old_master_number)
	{
		//old master == new master
		//no migration
		query->push_back(MYSELF);
		send_query(query);
		response=get_query_response(query);
		response->pop(ret);
	}
	else
	{
		//old master != new master
		//migration
		query->push_back(new_master_number);
		send_query(query);
		response=get_query_response(query);
		response->pop(ret);
		response_dequeue(response);

		extended_IO_task* query=allocate_new_query(new_master_handle);
		query->push_back(RENAME_MIGRATING);
		query->push_back_string(new_name);
		query->push_back(old_master_number);
		send_query(query);
		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
	}
	response_dequeue(response);
	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		errno=-ret;
		return -errno;
	}
}

int CBB_client::
_mkdir(	const char* 	path,
	mode_t		mode)
throw(std::runtime_error)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	comm_handle_t master_handle=_get_master_handle_from_path(path);

	extended_IO_task* query=allocate_new_query(master_handle);
	query->push_back(MKDIR); 
	query->push_back_string(path);
	query->push_back(mode);
	send_query(query);

	extended_IO_task* response=get_query_response(query);
	response->pop(ret);
	response_dequeue(response);
	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		errno=-ret;
		return -errno;
	}
}

off64_t CBB_client::
_tell(int fd)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	return _file_list.at(fid).current_point;
}

bool CBB_client::
_interpret_path(const char *path)
{
	if(nullptr == mount_point)
	{
		return false;
	}
	if(!strncmp(path, mount_point, strlen(mount_point)))
	{
		return true;
	}
	return false;
}

bool CBB_client::
_interpret_fd(int fd)
{
	if(fd<INIT_FD)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void CBB_client::
_format_path(	const char* 	path, 
		string&		formatted_path)
{
	char *real_path=static_cast<char *>(malloc(PATH_MAX));
	realpath(path, real_path);

	formatted_path=string(real_path);
	free(real_path);
	return;
}

void CBB_client::
_get_relative_path(const string& formatted_path,
		string& 	 relative_path)
{
	const char *pointer=formatted_path.c_str()+strlen(mount_point);
	relative_path=std::string(pointer);
	return;
}

size_t CBB_client::
_get_file_size(int fd)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	return _file_list.at(fid).get_file_metadata().st_size;
}

int CBB_client::
_get_local_attr(const char*  path,
		struct stat* file_stat)
{
	std::string string_path(path);

	try
	{
		file_meta* file_meta_p=_path_file_meta_map.at(string_path);
		memcpy(file_stat, &file_meta_p->file_stat, sizeof(struct stat));
		return SUCCESS;
	}
	catch(std::out_of_range& e)
	{
		return FAILURE;
	}
}

int CBB_client::
_truncate(const char* 	path,
	  off64_t 	size)
throw(std::runtime_error)
{
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int master_number =_get_master_number_from_path(path);
	int ret		  =master_number;
	extended_IO_task* response=nullptr;
	comm_handle_t master_handle=
		_get_master_handle_from_master_number(master_number);

	do{
		master_number=ret;
		extended_IO_task* query=allocate_new_query(master_handle);
		query->push_back(TRUNCATE); 
		query->push_back_string(path);
		query->push_back(size);
		send_query(query);

		response=get_query_response(query);
		response->pop(ret);
		master_handle = _get_master_handle_from_master_number(ret);
	}while(master_number != ret && response_dequeue(response));
	response->pop(ret);
	response_dequeue(response);
	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		errno=-ret;
		return -errno;
	}
}

int CBB_client::
_ftruncate(int fd, off64_t size)
throw(std::runtime_error)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	opened_file_info& file=_file_list.at(fid);
	file.file_meta_p->file_stat.st_size=size;
	_update_access_time(fd);
	return _truncate(file.file_meta_p->it->first.c_str(), size);
}

int CBB_client::
open(const char* path,
		int 	 flag,
		mode_t 	 mode)
{
	int ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_open(path, flag, mode);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

ssize_t CBB_client::
read(int 	fd, 
		void* 	buffer,
		size_t 	size)
{
	ssize_t ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			ret=_read(fd, buffer, size);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

ssize_t CBB_client::
write(int 	  fd,
		const void* buffer,
		size_t 	  size)
{
	ssize_t ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			ret=_write(fd, buffer, size);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

int CBB_client::
close(int fd)
{
	int ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_close(fd);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

int CBB_client::
flush(int fd)
{
	int ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_flush(fd);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

off64_t CBB_client::lseek(int fd, off64_t offset, int whence)
{
	return _lseek(fd, offset, whence);
}

int CBB_client::
getattr(const char *path, struct stat* fstat)
{
	int ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_getattr(path, fstat);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

int CBB_client::
readdir(const char* path, dir_t& dir)
{
	int ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_readdir(path, dir);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

int CBB_client::
unlink(const char* path)
{
	int ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_unlink(path);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

int CBB_client::
rmdir(const char* path)
{
	int ret=0;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_rmdir(path);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

int CBB_client::
access(const char* path, int mode)
{
	int ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_access(path, mode);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

int CBB_client::
stat(const char* path, struct stat* buf)
{
	int ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=remote_stat(path, buf);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

int CBB_client::
rename(const char* old_name,
		const char* new_name)
{
	int ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_rename(old_name, new_name);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

int CBB_client::
mkdir(const char* path, mode_t mode)
{
	int ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_mkdir(path, mode);
			//end_recording();
			break;

		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

int CBB_client::
truncate(const char*path, off64_t size)
{
	int ret=0;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_truncate(path, size);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

int CBB_client::
ftruncate(int fd, off64_t size)
{
	int ret=FAILURE;
	for(int i=CBB_COMM_RETRY ; i > 0 ;i--)
	{
		try
		{
			//start_recording();
			ret=_ftruncate(fd, size);
			//end_recording();
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("handle error, retry %d\n", i);
		}
	}
	return ret;
}

off64_t CBB_client::
tell(int fd)
{
	return _tell(fd);
}

int CBB_client::
_report_IOnode_failure(comm_handle_t handle)
{
	SCBB* corresponding_SCBB=_find_scbb_by_handle(handle);
	if(nullptr == corresponding_SCBB)
	{
		return FAILURE;
	}
	int master_number=0;
	ssize_t IOnode_id=0;
	//_parse_master_IOnode_id(master_IOnode_id, master_number, IOnode_id);
	comm_handle_t master_handle=master_handle_list.at(master_number).get_master_handle();
	_DEBUG("report IOnode failure to master %d, IOnode id %ld\n", master_number, IOnode_id);

	extended_IO_task* query=allocate_new_query(master_handle);
	query->push_back(IONODE_FAILURE);
	query->push_back(IOnode_id);
	send_query(query);

	return SUCCESS;
}

SCBB* CBB_client::
_find_scbb_by_handle(comm_handle_t handle)
{
	for(auto& scbb:master_handle_list)
	{
		for(auto& IOnode:scbb.IOnode_list)
		{
			if(compare_handle(handle, &IOnode.second))
			{
				return &scbb;
			}
		}
	}
	return nullptr;
}

off64_t CBB_client::
find_start_point(const _block_list_t& 	blocks,
		off64_t 		start_point,
		ssize_t 		update_size)
{
	off64_t ret=begin(blocks)->first;
	off64_t end_start_point=start_point + update_size;
	for(const auto& block:blocks)
	{
		if( end_start_point < block.first)
		{
			return ret;
		}
		else
		{
			ret=block.first;
		}
	}
	return ret;
}
