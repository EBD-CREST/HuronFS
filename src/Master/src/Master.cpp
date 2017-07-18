/*OB`
 * Master.cpp * *  Created on: Aug 8, 2014 *      Author: xtq
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

#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sched.h>
#include <ifaddrs.h>

#include "Master.h"
#include "CBB_internal.h"
#include "Comm_api.h"

using namespace CBB::Common;
using namespace CBB::Master;
using namespace std;

const char *Master::MASTER_MOUNT_POINT	="HUFS_MASTER_MOUNT_POINT";
//const char *Master::MASTER_NUMBER	="HUFS_MASTER_MY_ID";
//const char *Master::MASTER_TOTAL_NUMBER	="HUFS_MASTER_TOTAL_NUMBER";
const char *Master::MASTER_BACKUP_POINT	="HUFS_MASTER_BACKUP_POINT";
const char *Master::MASTER_IP_LIST	="HUFS_MASTER_IP_LIST";

Master::Master()throw(CBB_configure_error):
	//base class
	Server(MASTER_QUEUE_NUM, MASTER_PORT), 
	CBB_heart_beat(),
	//fields
	_registered_IOnodes(IOnode_t()), 
	_file_stat_pool(),
	_buffered_files(), 
	_IOnode_handle(IOnode_handle_t()), 
	_file_number(0), 
	_node_id_pool(new bool[MAX_IONODE]), 
	_file_no_pool(new bool[MAX_FILE_NUMBER]), 
	_current_node_number(0), 
	_current_file_no(0),
	_mount_point(),
	metadata_backup_point(),
	master_number(0),
	master_total_size(0),
	_current_IOnode(_registered_IOnodes.end())
{
	const char *master_mount_point		=getenv(MASTER_MOUNT_POINT);
	const char *master_backup_point_string 	=getenv(MASTER_BACKUP_POINT);
	const char *master_ip_list_string	=getenv(MASTER_IP_LIST);
	//const char *master_number_string	=getenv(MASTER_NUMBER);
	//const char *master_total_size_string	=getenv(MASTER_TOTAL_NUMBER);
	
	memset(_node_id_pool, 0, MAX_IONODE*sizeof(bool)); 
	memset(_file_no_pool, 0, MAX_FILE_NUMBER*sizeof(bool)); 

	if(nullptr == master_mount_point)
	{
		throw CBB_configure_error("please set master mount point");
	}
	if(nullptr == master_backup_point_string)
	{
		throw CBB_configure_error("please set master backup point");
	}
	if(nullptr == master_ip_list_string)
	{
		throw CBB_configure_error("please set master ip list");
	}

	if(FAILURE == _set_master_number_size(master_ip_list_string,
			        master_number,
			       	master_total_size))
	{
		throw CBB_configure_error("master ip error");
	}


	_mount_point	     =string(master_mount_point);
	metadata_backup_point=string(master_backup_point_string);
	//master_number	     =atoi(master_number_string);
	//master_total_size    =atoi(master_total_size_string);

	_setup_queues();

	_init_server();
	_buffer_all_meta_data_from_remote(master_mount_point); //throw CBB_configure_error
}

CBB::CBB_error Master::_setup_queues()
{
	CBB_heart_beat::set_queues(Server::get_communication_input_queue(
				MASTER_HEART_BEAT_QUEUE_NUM),
			Server::get_communication_output_queue(
				MASTER_HEART_BEAT_QUEUE_NUM));
	return SUCCESS;
}

Master::~Master()
{
	Server::stop_server(); 
	for(IOnode_t::iterator it=_registered_IOnodes.begin();
			it!=_registered_IOnodes.end();++it)
	{
		
		Send(&(it->second->handle), I_AM_SHUT_DOWN);
		Close(&(it->second->handle));
		delete it->second;
	}
	for(File_t::iterator it=_buffered_files.begin();
			it!=_buffered_files.end();++it)
	{
		delete it->second;
	}
	delete _node_id_pool;
	delete _file_no_pool; 
}

CBB::CBB_error Master::start_server()
{
	Server::start_server();
	while(true)
	{
		heart_beat_check();
		sleep(HEART_BEAT_INTERVAL);
	}
	return SUCCESS;
}

void Master::stop_server()
{
	Server::stop_server();
}

CBB::CBB_error Master::remote_task_handler(remote_task* new_task)
{
	int request=new_task->get_task_id();
	switch(request)
	{
		case RENAME:
			_remote_rename(new_task);	break;
		case RM_DIR:
			_remote_rmdir(new_task);	break;
		case UNLINK:
			_remote_unlink(new_task);	break;
		case MKDIR:
			_remote_mkdir(new_task);	break;
	}
	return SUCCESS;
}

CBB::CBB_error Master::_parse_request(extended_IO_task* new_task)
{
	int request	=0;
	int ans		=SUCCESS; 
	new_task->pop(request); 
	_DEBUG("new request %d\n", request);

	switch(request)
	{
		case REGISTER:
			_parse_register_IOnode(new_task);	break;
		case NEW_CLIENT:
			_parse_new_client(new_task);	break;
		case OPEN_FILE:
			_parse_open_file(new_task);	break; 
		case READ_FILE:
			_parse_read_file(new_task);	break;
		case WRITE_FILE:
			_parse_write_file(new_task);	break;
		case FLUSH_FILE:
			_parse_flush_file(new_task);	break;
		case CLOSE_FILE:
			_parse_close_file(new_task);	break;
		case GET_ATTR:
			_parse_attr(new_task);		break;
		case READ_DIR:
			_parse_readdir(new_task);	break;
		case RM_DIR:
			_parse_rmdir(new_task);		break;
		case UNLINK:
			_parse_unlink(new_task);	break;
		case ACCESS:
			_parse_access(new_task);	break;
		case RENAME:
			_parse_rename(new_task);	break;
		case RENAME_MIGRATING:
			_parse_rename_migrating(new_task);break;
		case MKDIR:
			_parse_mkdir(new_task);		break;
		case TRUNCATE:
			_parse_truncate_file(new_task);	break;
		case CLOSE_CLIENT:
			_parse_close_client(new_task);	break;
		case NODE_FAILURE:
			_parse_node_failure(new_task);	break;
		case IONODE_FAILURE:
			_parse_IOnode_failure(new_task);break;
	}
	return ans; 
}

//R: uri	:char
//R: total memory: size_t
//S: node_id: ssize_t
CBB::CBB_error Master::_parse_register_IOnode(extended_IO_task* new_task)
{
	size_t 	total_memory	=0;
	char*	uri		=nullptr;

	new_task->pop(total_memory);
	new_task->pop_string(&uri);

	std::string uri_string=std::string(uri);
	_LOG("register IOnode uri=%s\n",uri);

	extended_IO_task* output=init_response_task(new_task);
	output->push_back(_add_IOnode(uri, total_memory, new_task->get_handle()));
	output_task_enqueue(output);
	_DEBUG("total memory %lu\n", total_memory);
	return SUCCESS;
}

//S: SUCCESS: int
CBB::CBB_error Master::_parse_new_client(extended_IO_task* new_task)
{
	char * uri=nullptr;	
	new_task->pop_string(&uri);
	_LOG("new client uri=%s\n", uri);

	extended_IO_task* output=init_response_task(new_task);

	output->push_back(SUCCESS);

	output_task_enqueue(output);
	return SUCCESS;
}

//rename-open issue
//R: file_path: char[]
//R: flag: int
//S: SUCCESS			errno: int
//S: file no: ssize_t
//S: block size: size_t
//S: attr: struct stat
CBB::CBB_error Master::_parse_open_file(extended_IO_task* new_task)
{
	char 	*file_path	=nullptr;
	int 	flag		=0;
	int 	ret		=SUCCESS;
	ssize_t file_no		=0;
	int 	exist_flag	=EXISTING;
	mode_t 	mode		=S_IFREG|0660;
	_LOG("request for open file\n");

	new_task->pop_string(&file_path); 
	extended_IO_task* output=nullptr;
	std::string str_file_path=std::string(file_path);
	file_stat_pool_t::iterator it=_file_stat_pool.find(str_file_path);

	new_task->pop(flag); 

	if(flag & O_CREAT)
	{
		new_task->pop(mode);
		exist_flag=NOT_EXIST;
		flag &= ~(O_CREAT | O_TRUNC);
	}
	if((_file_stat_pool.end() != it))
	{
		Master_file_stat& file_stat=it->second;
		if(file_stat.is_external())
		{
			extended_IO_task* output=init_response_task(new_task);
			output->push_back(file_stat.external_master);
			output_task_enqueue(output);
			return SUCCESS;
		}
	}
	try
	{
		_open_file(file_path, flag, file_no, exist_flag, mode); 
		open_file_info *opened_file=_buffered_files.at(file_no);
		size_t block_size=opened_file->block_size;
		_DEBUG("file path= %s, file no=%ld\n", file_path, file_no);
		output=init_response_task(new_task);
		output->push_back(SUCCESS);
		output->push_back(file_no);
		output->push_back(block_size);
		Send_attr(output, &opened_file->get_stat());
	}
	catch(std::runtime_error &e)
	{
		_DEBUG("%s\n",e.what());
		output=init_response_task(new_task);
		output->push_back(UNKNOWN_ERROR);
		ret=FAILURE;
	}
	catch(std::invalid_argument &e)
	{
		_DEBUG("%s\n",e.what());
		output=init_response_task(new_task);
		output->push_back(FILE_NOT_FOUND);
		ret=FAILURE;
	}
	catch(std::bad_alloc &e)
	{
		output=init_response_task(new_task);
		output->push_back(TOO_MANY_FILES);
		ret=FAILURE;
	}
	output_task_enqueue(output);
	return ret;
}

//R: file no: ssize_t
//S: SUCCESS			errno: int
//R: start point: off64_t
//R: size: size_t
//S: block_info
CBB::CBB_error Master::_parse_read_file(extended_IO_task* new_task)
{
	ssize_t 	file_no		=0;
	size_t 		size		=0;
	int 		ret		=0;
	off64_t 	start_point	=0;
	open_file_info 	*file		=nullptr; 
	_LOG("request for reading\n");

	new_task->pop(file_no); 
	extended_IO_task* output=init_response_task(new_task);
	try
	{
		file=_buffered_files.at(file_no);
		new_task->pop(start_point);
		new_task->pop(size);

		block_list_t block_list;
		node_info_pool_t node_pool;
		//_get_blocks_for_IO(start_point, size, *file, IO_blocks);
		_select_node_block_set(*file, start_point, size, node_pool, block_list);
		output=init_response_task(new_task);
		_send_block_info(output, file->get_stat().st_size, node_pool, block_list);
		ret=SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		output->push_back(OUT_OF_RANGE);
		ret=FAILURE; 
	}
	output_task_enqueue(output);
	return ret;

}

//R: file no: ssize_t
//S: SUCCESS			errno: int
//R: attr: struct stat
//R: start point: off64_t
//R: size: size_t
//S: send_block
CBB::CBB_error Master::_parse_write_file(extended_IO_task* new_task)
{
	_LOG("request for writing\n");
	ssize_t file_no		=0;
	size_t 	size		=0;
	size_t 	original_size	=0;
	int 	ret		=0;
	off64_t start_point	=0;

	new_task->pop(file_no);
	_DEBUG("file no=%ld\n", file_no);
	extended_IO_task* output=nullptr;
	try
	{
		open_file_info &file=*_buffered_files.at(file_no);
		std::string real_path=_get_real_path(file.get_path());
		struct stat& file_status=file.get_stat();
		original_size=file_status.st_size;
		Recv_attr(new_task, &file.get_stat());
		if(static_cast<ssize_t>(original_size) > file_status.st_size)
		{
			file_status.st_size=original_size;
		}

		new_task->pop(start_point);
		new_task->pop(size);
		_DEBUG("real_path=%s, file_size=%ld\n", real_path.c_str(), file_status.st_size);

		block_list_t block_list;
		node_info_pool_t node_pool;

		_allocate_new_blocks_for_writing(file, start_point, size);
		_select_node_block_set(file, start_point, size, node_pool, block_list);
		output=init_response_task(new_task);
		_send_block_info(output, file_status.st_size, node_pool, block_list);
		ret=SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		output=init_response_task(new_task);
		output->push_back(OUT_OF_RANGE);
		ret=FAILURE;
	}
	output_task_enqueue(output);
	return ret;
}

CBB::CBB_error
Master::_select_node_block_set(open_file_info& 	file,
			       off64_t 		start_point,
			       size_t 	   	size,
			       node_info_pool_t &node_info_pool,
			       block_list_t& 	block_list)const
{
	node_info_pool.insert(file.primary_replica_node); //only return the main IOnode
	off64_t block_start_point=get_block_start_point(start_point);
	auto block=file.block_list.find(block_start_point);
	for(ssize_t remaining_size=size;0 < remaining_size;remaining_size -= BLOCK_SIZE)
	{
		block_list.insert(make_pair(block->first, block->second));
		//node_info_pool.insert(block->second);
		block_start_point += BLOCK_SIZE;
		block++;
	}
	return SUCCESS;
}

//R: file no: ssize_t
//S: SUCCESS			errno: int
CBB::CBB_error Master::_parse_flush_file(extended_IO_task* new_task)
{
	_LOG("request for writing\n");
	ssize_t file_no	=0;
	int 	ret	=0;

	new_task->pop(file_no);
	extended_IO_task* output=init_response_task(new_task);
	try
	{
		open_file_info &file=*_buffered_files.at(file_no);
		output->push_back(SUCCESS);
		output_task_enqueue(output);
		_DEBUG("write back request to IOnode %ld, file_no %ld\n", file.primary_replica_node->node_id, file_no);
		extended_IO_task* IOnode_output=allocate_output_task(_get_my_thread_id());
		IOnode_output->set_handle(file.primary_replica_node->handle);
		IOnode_output->push_back(FLUSH_FILE);
		IOnode_output->push_back(file_no);
		output_task_enqueue(IOnode_output);

		ret=SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		extended_IO_task* output=init_response_task(new_task);
		output->push_back(FAILURE);
		output_task_enqueue(output);
		ret=FAILURE;
	}
	return ret;
}

//R: file no: ssize_t
//S: SUCCESS			errno: int
CBB::CBB_error Master::_parse_close_file(extended_IO_task* new_task)
{
	ssize_t 	  file_no	=0;
	extended_IO_task  *output	=init_response_task(new_task);
	open_file_info    *file		=nullptr;

	_LOG("request for closing file\n");
	new_task->pop(file_no);
	try
	{
		_LOG("file no %ld\n", file_no);
		file=_buffered_files.at(file_no);
		output->push_back(SUCCESS);
		output_task_enqueue(output);
	}
	catch(std::out_of_range &e)
	{
		output->push_back(FAILURE);
		output_task_enqueue(output);
		return FAILURE;
	}
	_DEBUG("write back request to IOnode %ld\n", file->primary_replica_node->node_id);
	extended_IO_task* IOnode_output=allocate_output_task(_get_my_thread_id());
	IOnode_output->set_handle(file->primary_replica_node->handle);
	IOnode_output->push_back(CLOSE_FILE);
	IOnode_output->push_back(file_no);
	output_task_enqueue(IOnode_output);
	return SUCCESS;
}

//R: file_path: char[]
//S: SUCCESS			errno: int
//S: attr: struct stat
CBB::CBB_error Master::_parse_attr(extended_IO_task* new_task)
{
	_DEBUG("requery for File info\n");
	std::string 	  real_path;
	std::string 	  relative_path;
	int 	    	  ret	=0;
	extended_IO_task *output=init_response_task(new_task);

	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	_DEBUG("file path=%s\n", real_path.c_str());
	try
	{
		//_file_stat_pool.rd_lock();
		Master_file_stat& file_stat=_file_stat_pool.at(relative_path);
		//_file_stat_pool.unlock();
		if(file_stat.is_external())
		{
			output->push_back(file_stat.external_master);
			ret=SUCCESS;
		}
		else
		{
			output->push_back(master_number);
			struct stat& status=file_stat.get_status();
			output->push_back(SUCCESS);
			Send_attr(output, &status);
			_DEBUG("%ld\n", status.st_size);
			ret=SUCCESS;
		}
	}
	catch(std::out_of_range &e)
	{
		output->push_back(master_number);
		output->push_back(-ENOENT);
		ret=FAILURE;
	}
	output_task_enqueue(output);
	return ret;
}

//R: file path: char[]
//S: SUCCESS				errno: int
//S: dir_count: int
//for dir_count
	//S: file name: char[]
//S: SUCCESS: int
CBB::CBB_error Master::_parse_readdir(extended_IO_task* new_task)
{
	_DEBUG("requery for dir info\n");
	std::string relative_path;
	std::string real_path;

	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	_DEBUG("file path=%s\n", real_path.c_str());
	dir_t files;
	_get_file_stat_from_dir(relative_path, files);
	
	extended_IO_task* output=init_response_task(new_task);
	output->push_back(SUCCESS);
	output->push_back(static_cast<int>(files.size()));
	output->init_response_large_transfer(new_task, RMA_WRITE);
	for(dir_t::const_iterator it=files.begin();
			it!=files.end();++it)
	{
		output->push_string_prealloc(*it);
	}
	output->setup_prealloc_buffer();

	output_task_enqueue(output);
	return SUCCESS;
}

//R: file path: char[]
//S: SUCCESS				errno: int
CBB::CBB_error Master::_parse_unlink(extended_IO_task* new_task)
{
	_LOG("request for unlink\n"); 
	int 	    		ret;
	std::string 		relative_path;
	std::string 		real_path;
	extended_IO_task 	*output=nullptr;

	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	_LOG("path=%s\n", real_path.c_str());
	if(SUCCESS == _remove_file(relative_path))
	{
		output=init_response_task(new_task);
		output->push_back(SUCCESS);
		ret=SUCCESS;
	}
	else
	{
		output=init_response_task(new_task);
		output->push_back(-ENOENT);
		ret=FAILURE;
	}

	output_task_enqueue(output);
	return ret;
}

//R: file path: char[]
//S: SUCCESS				errno: int
int Master::_parse_rmdir(extended_IO_task* new_task)
{
	_LOG("request for rmdir\n");
	string 			file_path;
	string 			relative_path_string;
	int 	    		ret;
	extended_IO_task 	*output=init_response_task(new_task);

	Server::_recv_real_relative_path(new_task, file_path, relative_path_string);
	_LOG("path=%s\n", file_path.c_str());

	if(SUCCESS == _remove_file(relative_path_string))
	{
		output->push_back(SUCCESS);
		ret=SUCCESS;
	}
	else
	{
		output->push_back(-ENOENT);
		ret=FAILURE;
	}
	
	/*if(-1 != rmdir(file_path.c_str()))
	{
		output->push_back(SUCCESS);
		ret=SUCCESS;
	}
	else
	{
		output->push_back(-errno);
		ret=FAILURE;
	}*/
	output_task_enqueue(output);
	return ret;
}

//R: file path: char[]
//R: mode: int
//S: SUCCESS				errno: int
CBB::CBB_error Master::_parse_access(extended_IO_task* new_task)
{
	int 		mode;
	int		ret;
	std::string 	real_path;
	std::string 	relative_path;

	_LOG("request for access\n");
	extended_IO_task* output=init_response_task(new_task);
	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	new_task->pop(mode);
	_LOG("path=%s, mode=%d\n", real_path.c_str(), mode);
	try
	{
		//_file_stat_pool.rd_lock();
		const Master_file_stat& file_stat=_file_stat_pool.at(relative_path);
		//_file_stat_pool.unlock();
		
		if(file_stat.is_external())
		{
			output->push_back(file_stat.external_master);
			ret=SUCCESS;
		}
		else
		{
			output->push_back(master_number);
			output->push_back(SUCCESS);
			ret=SUCCESS;
		}
	}
	catch(std::out_of_range &e)
	{
		output->push_back(master_number);
		output->push_back(-ENOENT);
		ret=FAILURE;
	}
	output_task_enqueue(output);
	return ret;
}

//R: file path: char[]
//R: mode: mode_t
//S: SUCCESS				errno: int
CBB::CBB_error Master::_parse_mkdir(extended_IO_task* new_task)
{
	_LOG("request for mkdir\n");
	mode_t 			mode	=0;
	extended_IO_task	*output	=init_response_task(new_task);
	std::string 		real_path;
	std::string 		relative_path_string;

	_recv_real_relative_path(new_task, real_path, relative_path_string);
	new_task->pop(mode);
	_LOG("path=%s\n", real_path.c_str());
	//make internal dir
	mode|=S_IFDIR;
	_create_new_file_stat(relative_path_string.c_str(), NOT_EXIST, mode);
	output_task_enqueue(output);
	return SUCCESS;
}

CBB::CBB_error Master::_remove_file(const std::string& file_path)
{
	file_stat_pool_t::iterator file_stat=_file_stat_pool.find(file_path);

	//if file exists
	if(end(_file_stat_pool) != file_stat)
	{
		open_file_info* opened_file=file_stat->second.opened_file_info;
		//if the file has been opened
		if(NULL != opened_file)
		{
			_remove_open_file(opened_file);
		}

		_file_stat_pool.erase(file_stat);
		return SUCCESS;
	}
	//if not
	else
	{
		return FAILURE;
	}
}

//R: file path: char[]
//R: file path: char[]
//S: SUCCESS				errno: int
CBB::CBB_error Master::_parse_rename(extended_IO_task* new_task)
{
	int	new_master=0;
	string 	old_real_path;
	string 	old_relative_path;
	string	new_real_path;
	string	new_relative_path;

	_recv_real_relative_path(new_task, old_real_path, old_relative_path);
	_recv_real_relative_path(new_task, new_real_path, new_relative_path); 
	_LOG("request for rename file\n");
	_LOG("old file path=%s, new file path=%s\n", old_real_path.c_str(), new_real_path.c_str());
	//unpaired
	new_task->pop(new_master);
	file_stat_pool_t::iterator required_file=_file_stat_pool.find(old_relative_path);
	if(MYSELF == new_master)
	{
		//remove the file which has the same name as the new file name
		_remove_file(new_relative_path);
		if(end(_file_stat_pool) != required_file)
		{
			_rename_local_file(required_file->second, new_relative_path);
			//remove local file_stat
			_file_stat_pool.erase(required_file);
		}
	}
	else
	{
		if( end(_file_stat_pool) != required_file)
		{
			required_file->second.external_flag = RENAMED;
			required_file->second.external_name = new_relative_path;
		}
	}
	extended_IO_task* output=init_response_task(new_task);
	output->push_back(SUCCESS);
	output_task_enqueue(output);

	return SUCCESS;
}

CBB::CBB_error
Master::_rename_local_file(Master_file_stat 	&file_stat,
	      		   const string 	&new_relative_path)
{
	_DEBUG("rename local file\n");

	file_stat_pool_t::iterator new_file=_file_stat_pool.insert(std::make_pair(new_relative_path, file_stat)).first;
	new_file->second.full_path=new_file;

	if(nullptr != new_file->second.opened_file_info)
	{
		open_file_info* opened_file = new_file->second.opened_file_info;
		opened_file->file_status=&(new_file->second);

		_send_rename_request(opened_file->primary_replica_node, opened_file->file_no, new_relative_path);
		for(auto node:opened_file->IOnodes_set)
		{
			_send_rename_request(node, opened_file->file_no, new_relative_path);
		}
	}
	return SUCCESS;
}

void Master::_send_rename_request(node_info	*node,
				  ssize_t 	file_no,
				  const string 	&new_relative_path)
{
	extended_IO_task* output=allocate_output_task(_get_my_thread_id());
	output->set_handle(node->handle);
	output->push_back(RENAME);
	output->push_back(file_no);
	output->push_back_string(new_relative_path.c_str(), new_relative_path.size());
	output_task_enqueue(output);
}


CBB::CBB_error Master::_parse_close_client(extended_IO_task* new_task)
{
	_LOG("close client\n");
	comm_handle_t handle=new_task->get_handle();
	Server::_delete_handle(handle);
	Close(handle);
	return SUCCESS;
}

//R: file path: char[]
//R: file size: off64_t
//S: SUCCESS				errno: int
CBB::CBB_error Master::_parse_truncate_file(extended_IO_task* new_task)
{
	int 			ret	=0;
	ssize_t 		size	=0;
	extended_IO_task	*output	=nullptr;
	std::string 		real_path;
	std::string 		relative_path;

	_LOG("request for truncate file\n");
	_recv_real_relative_path(new_task, real_path, relative_path);
	new_task->pop(size);
	_LOG("path=%s\n", real_path.c_str());

	try
	{
		Master_file_stat& file_stat=_file_stat_pool.at(relative_path);
		if(file_stat.is_external())
		{
			output=init_response_task(new_task);
			output->push_back(file_stat.external_master);
			ret=SUCCESS;
		}
		else
		{
			ssize_t fd=file_stat.get_fd();
			open_file_info* file=file_stat.opened_file_info;
			if(file->get_stat().st_size > size)
			{
				//send free to IOnode;
				off64_t block_start_point=get_block_start_point(size);

				_send_truncate_request(file->primary_replica_node, fd, block_start_point, size);
				for(auto& node:file->IOnodes_set)
				{
					_send_truncate_request(node, fd, block_start_point, size);
				}
				block_start_point += BLOCK_SIZE;
				while(static_cast<ssize_t>(block_start_point + BLOCK_SIZE) <= size)
				{
					_DEBUG("remove block %ld\n", block_start_point);
					file->block_list.erase(block_start_point);
					block_start_point+=BLOCK_SIZE;
				}
			}
			file->get_stat().st_size=size;
			//_update_backup_file_size(relative_path, size);
			output=init_response_task(new_task);
			output->push_back(master_number);
			output->push_back(SUCCESS);
			ret=SUCCESS;
		}
	}
	catch(std::out_of_range &e)
	{
		output=init_response_task(new_task);
		output->push_back(master_number);
		output->push_back(-ENOENT);
		ret=FAILURE;
	}
	output_task_enqueue(output);
	return ret;
}

void Master::_send_truncate_request(node_info 	*node,
				    ssize_t 	fd,
				    off64_t 	block_start_point,
				    ssize_t 	size)
{
	extended_IO_task* output=nullptr;

	output=allocate_output_task(_get_my_thread_id());
	output->set_handle(node->handle);
	output->push_back(TRUNCATE);
	output->push_back(fd);
	output->push_back(block_start_point);
	output->push_back(size-block_start_point);
	output_task_enqueue(output);
}

CBB::CBB_error Master::_parse_rename_migrating(extended_IO_task* new_task)
{
	int 	old_master;
	string 	real_path;
	string 	relative_path;

	_recv_real_relative_path(new_task, real_path, relative_path);
	new_task->pop(old_master);
	_LOG("request for truncate file\n");
	_LOG("path=%s\n", relative_path.c_str());
	extended_IO_task* output=init_response_task(new_task);
	file_stat_pool_t::iterator it=_file_stat_pool.find(relative_path);
	if(_file_stat_pool.end() == it)
	{
		it=_file_stat_pool.insert(std::make_pair(relative_path, Master_file_stat())).first; 
		it->second.full_path=it;
	}
	Master_file_stat& file_stat=it->second;
	file_stat.external_flag=EXTERNAL;
	file_stat.external_master=old_master;
	output->push_back(SUCCESS);
	output_task_enqueue(output);
	return SUCCESS;
}

CBB::CBB_error Master::_parse_node_failure(extended_IO_task* new_task)
{
	comm_handle_t	handle		=new_task->get_handle();
	node_info* 	failed_node 	=_get_node_info_from_handle(handle);

	_LOG("parsing node failure\n");
	if(nullptr != failed_node)
	{
		return _IOnode_failure_handler(failed_node);
	}
	else
	{
		return _close_client(handle);
	}
}

CBB::CBB_error Master::_IOnode_failure_handler(node_info* IOnode_info)
{
	_DEBUG("handle IOnode failure node id=%ld\n", IOnode_info->node_id);
	_send_remove_IOnode_request(IOnode_info);
	_recreate_replicas(IOnode_info);
	return _unregister_IOnode(IOnode_info);
}

CBB::CBB_error Master::_recreate_replicas(node_info* IOnode_info)
{
	_LOG("recreating new replicas\n");

	if(1 == _registered_IOnodes.size())
	{
		//only one node remains
		_remove_IOnode_buffered_file(IOnode_info);
		return SUCCESS;
	}

	//create replacement for each file
	for(ssize_t file_no:IOnode_info->stored_files)
	{
		_DEBUG("recreate replica for file no %ld\n", file_no);

		open_file_info* file_info=nullptr;
		file_info=_buffered_files.at(file_no);	
	
		node_info* new_IOnode=_allocate_replace_IOnode(file_info->IOnodes_set, IOnode_info);
		file_info->IOnodes_set.erase(IOnode_info); //replace IOnode in node list of that file

		if(nullptr != new_IOnode)
		{
			//we still get replica
			if(IOnode_info == file_info->primary_replica_node)
			{
				file_info->primary_replica_node=*begin(file_info->IOnodes_set); //select the first replica as primary 
				file_info->IOnodes_set.erase(file_info->primary_replica_node); //remove primary from replica list
				file_info->IOnodes_set.insert(new_IOnode); //insert new replica

				_resend_replica_nodes_info_to_new_node(file_info, file_info->primary_replica_node, new_IOnode); 
			}
			else
			{
				file_info->IOnodes_set.insert(new_IOnode);
				_replace_replica_nodes_info(file_info, new_IOnode, IOnode_info);
			}
			_send_open_request_to_IOnodes(*file_info, new_IOnode, file_info->block_list, file_info->IOnodes_set);
		}
		else
		{
			if(IOnode_info == file_info->primary_replica_node)
			{
				if(0 != file_info->IOnodes_set.size())
				{
					file_info->primary_replica_node=*begin(file_info->IOnodes_set); //select the first replica as primary 
					file_info->IOnodes_set.erase(file_info->primary_replica_node); //remove primary from replica list
				}
				else
				{
					_DEBUG("no more replica, lose of data\n");
				}
			}
		}

	}
	return SUCCESS;
}

CBB::CBB_error 
Master::_resend_replica_nodes_info_to_new_node(open_file_info	*file_info,
					       node_info	*primary_replica_node,
					       node_info 	*new_node)
{
	_LOG("send replica info to new main node\n");

	extended_IO_task* output=allocate_output_task(_get_my_thread_id());
	output->set_handle(primary_replica_node->handle);
	output->push_back(PROMOTED_TO_PRIMARY_REPLICA);
	output->push_back(file_info->file_no);
	output->push_back(new_node->node_id);
	_send_replica_nodes_info(output, file_info->IOnodes_set);
	output_task_enqueue(output);

	return SUCCESS;
}

CBB::CBB_error
Master::_replace_replica_nodes_info(open_file_info	*file_info,
				    node_info		*new_IOnode,
				    node_info		*replaced_info)
{
	_LOG("send replica info to new main node\n");

	extended_IO_task* output=allocate_output_task(_get_my_thread_id());
	output->set_handle(file_info->primary_replica_node->handle);
	output->push_back(REPLACE_REPLICA);
	output->push_back(file_info->file_no);
	output->push_back(replaced_info->node_id);
	output->push_back(new_IOnode->node_id);
	output->push_back_string(new_IOnode->uri);
	output_task_enqueue(output);

	return SUCCESS;
}

CBB::CBB_error Master::_send_remove_IOnode_request(node_info* removed_IOnode)
{
	_LOG("send remove IOnode request to each IOnode\n");

	for(auto& IOnode:_registered_IOnodes)
	{
		if(removed_IOnode != IOnode.second)
		{
			//send the remove request to all other IOnodes
			extended_IO_task* output=allocate_output_task(_get_my_thread_id());
			output->set_handle(IOnode.second->handle);
			output->push_back(REMOVE_IONODE);
			output->push_back(removed_IOnode->node_id);
			output_task_enqueue(output);
		}
	}

	return SUCCESS;
}

node_info* Master::_allocate_replace_IOnode(node_info_pool_t& 	node_pool,
					    node_info* 		old_node)
{
	node_info* new_node=nullptr;

	if(NUM_OF_REPLICA > node_pool.size())
	{
		return new_node;
	}
	do
	{
		new_node=_get_next_IOnode();
	}while(new_node == old_node
	       && end(node_pool) != node_pool.find(new_node));

	return new_node;
}

CBB::CBB_error Master::_unregister_IOnode(node_info* IOnode_info)
{
	comm_handle_t handle=&IOnode_info->handle;
	_LOG("unregister IOnode id %ld\n", IOnode_info->node_id);
	_IOnode_handle.erase(handle);
	
	_remove_IOnode(IOnode_info);
	return SUCCESS;
}

CBB::CBB_error Master::_remove_IOnode(node_info* IOnode_info)
{
	if(0 != _registered_IOnodes.size() && 
		_current_IOnode->first == IOnode_info->node_id)
	{
		//current_IOnode is the node to be removed
		//go to next IOnode
		_current_IOnode++;
	}

	_registered_IOnodes.erase(IOnode_info->node_id);
	delete IOnode_info;
	return SUCCESS;
}

CBB::CBB_error Master::_remove_IOnode_buffered_file(node_info* IOnode_info)
{
	file_no_pool_t& stored_files=IOnode_info->stored_files;
	for(auto &file_no:stored_files)
	{
		open_file_info* file_info=_buffered_files.at(file_no);	
		//remove IOnode no from node list in that file
		file_info->IOnodes_set.erase(IOnode_info);
		if(IOnode_info == file_info->primary_replica_node)
		{
			file_info->primary_replica_node=*begin(file_info->IOnodes_set);
		}
	}
	return SUCCESS;
}

CBB::CBB_error Master::_close_client(comm_handle_t handle)
{
	_LOG("closing client handle %p\n", handle);
	Server::_delete_handle(handle);
	return SUCCESS;
}
	
//S: count: int
//for count:
	//S: node id: ssize_t
	//S: node ip: char[]
//S: count: int
//for count:
	//S: start point: off64_t
	//S: node id: ssize_t
//S: SUCCESS: int
void Master::
_send_block_info(Common::extended_IO_task*	output,
	      	 size_t 			file_size,
		 const node_info_pool_t&	node_info_pool,
		 const block_list_t&		block_list)const
{
	output->push_back(SUCCESS);
	output->push_back(file_size);
	output->push_back(static_cast<int>(node_info_pool.size()));
	for(auto& node_info:node_info_pool)
	{
		output->push_back(node_info->node_id);
		const std::string& uri=node_info->uri;
		output->push_back_string(uri);
	}

	output->push_back(static_cast<int>(block_list.size()));

	for(auto& block:block_list)
	{
		_DEBUG("start point %ld, block size=%ld\n", block.first, block.second);
		output->push_back(block.first);
		output->push_back(block.second);
	}
	return; 
}

//S: APPEND_BLOCK: int
//S: file no: ssize_t
//R: ret
	//S: count: int
	//S: start point: off64_t
	//S: data size: size_t
	//S: SUCCESS: int
	//R: ret
/*
void Master::_send_append_request(ssize_t file_no,
		const node_block_map_t& append_blocks)
{
	for(node_block_map_t::const_iterator nodes_it=append_blocks.begin();
			nodes_it != append_blocks.end();++nodes_it)
	{
		extended_IO_task* output=allocate_output_task(0);
		output->set_handle(_registered_IOnodes.at(nodes_it->first)->handle);
		output->push_back(APPEND_BLOCK);
		output->push_back(file_no);
		output->push_back(static_cast<int>(nodes_it->second.size()));
		for(block_info_t::const_iterator block_it=nodes_it->second.begin();
				block_it != nodes_it->second.end();++block_it)
		{
			output->push_back(block_it->first);
			output->push_back(block_it->second);
		}
		output_task_enqueue(output);
	}
	return;
}*/

//S: APPEND_BLOCK: int
//S: file no: ssize_t
//R: ret
	//S: count: int
	//S: start point: off64_t
	//S: data size: size_t
	//S: SUCCESS: int
	//R: ret
CBB::CBB_error Master::
_send_open_request_to_IOnodes(struct open_file_info  &file,
			      node_info		     *current_node_info,
			      const block_list_t     &block_info,
		 	      const node_info_pool_t &IOnodes_set)
{
	//send read request to each IOnode
	//buffer requset, file_no, open_flag, exist_flag, file_path, start_point, block_size
	comm_handle_t 	 handle=&current_node_info->handle;
	extended_IO_task *output=allocate_output_task(_get_my_thread_id());

	output->set_handle(handle);
	output->push_back(OPEN_FILE);
	output->push_back(file.file_no); 
	output->push_back(file.flag);
	output->push_back(file.file_status->exist_flag);
	output->push_back_string(file.get_path().c_str(), file.get_path().length());
	_DEBUG("%s\n", file.get_path().c_str());

	output->push_back(static_cast<int>(block_info.size()));
	for(auto& block:block_info)
	{
		_DEBUG("block start_point=%ld\n", block.first);
		output->push_back(block.first);
		output->push_back(block.second);
	}
	//send other replicas info to the main replica
	if(current_node_info == file.primary_replica_node)
	{
		_send_replica_nodes_info(output, file.IOnodes_set);
	}
	else
	{
		output->push_back(SUB_REPLICA);
		output->push_back(static_cast<int>(0));
	}
	output_task_enqueue(output);
	return SUCCESS; 
}

CBB::CBB_error Master::
_send_replica_nodes_info(extended_IO_task 	*output,
			 const node_info_pool_t &IOnodes_set)
{
	output->push_back(MAIN_REPLICA);
	output->push_back(static_cast<int>(IOnodes_set.size()));
	for(auto& node_info:IOnodes_set)
	{
		output->push_back(node_info->node_id);
		output->push_back_string(node_info->get_uri());
	}
	return SUCCESS;
}

ssize_t Master::
_add_IOnode(const string  &node_uri,
	    size_t 	  total_memory,
	    comm_handle_t handle)
{
	ssize_t id=0; 
	if(-1 == (id=_get_node_id()))
	{
		return -1;
	}
	if(_registered_IOnodes.end() != _find_by_uri(node_uri))
	{
		return -1;
	}
	_registered_IOnodes.insert(std::make_pair(id,
				new node_info(id, node_uri, total_memory, handle)));
	node_info *node=_registered_IOnodes.at(id); 
	//get the actual handle address from inserted node info
	comm_handle_t node_handle=&node->handle;
	_IOnode_handle.insert(std::make_pair(node_handle, node)); 
	Server::_add_handle(handle); 
	return id; 
}

ssize_t Master::_delete_IOnode(comm_handle_t handle)
{
	node_info *node=_get_node_info_from_handle(handle);
	ssize_t id=node->node_id;
	_registered_IOnodes.erase(id);
	Server::_delete_handle(handle); 
	Close(handle); 
	_IOnode_handle.erase(&node->handle); 
	_node_id_pool[id]=false;
	delete node;
	return id;
}

const node_info_pool_t& Master::
_open_file(const char 	*file_path,
	   int 		flag,
	   ssize_t	&file_no,
	   int 		exist_flag,
	   mode_t 	mode)
throw(std::runtime_error,
	std::invalid_argument,
	std::bad_alloc)
{
	file_stat_pool_t::iterator it; 
	open_file_info *file=nullptr; 
	//file not buffered
	if(_file_stat_pool.end() ==  (it=_file_stat_pool.find(file_path)))
	{
		file_no=_get_file_no(); 
		if(-1 != file_no)
		{
			Master_file_stat* file_status=_create_new_file_stat(file_path, exist_flag, S_IFREG|mode);
			file=_create_new_open_file_info(file_no, flag, file_status);
		}
		else
		{
			throw std::bad_alloc();
		}
	}
	//file buffered
	else
	{
		Master_file_stat& file_stat=it->second;
		if(nullptr == file_stat.opened_file_info)
		{
			file_no=_get_file_no(); 
			file=_create_new_open_file_info(file_no, flag, &file_stat);
		}
		else
		{
			file=file_stat.opened_file_info;
			file_no=file->file_no;
		}
		++file->open_count;
	}
	return file->IOnodes_set;
}

ssize_t Master::_get_node_id()
{
	if(MAX_IONODE == _registered_IOnodes.size())
	{
		return -1;
	}
	for(; _current_node_number<MAX_IONODE; ++_current_node_number)
	{
		if(!_node_id_pool[_current_node_number])
		{
			_node_id_pool[_current_node_number]=true; 
			return _current_node_number; 
		}
	}
	for(_current_node_number=0;  _current_node_number<MAX_IONODE; ++_current_node_number)
	{
		if(!_node_id_pool[_current_node_number])
		{
			_node_id_pool[_current_node_number]=true; 
			return _current_node_number;
		}
	}
	return -1;
}

ssize_t Master::_get_file_no()
{
	if(MAX_FILE_NUMBER == _file_number)
	{
		return -1;
	}
	for(; _current_file_no<MAX_FILE_NUMBER; ++_current_file_no)
	{
		if(!_file_no_pool[_current_file_no])
		{
			_file_no_pool[_current_file_no]=true; 
			return _current_file_no; 
		}
	}
	for(_current_file_no=0;  _current_file_no<MAX_FILE_NUMBER; ++_current_file_no)
	{
		if(!_file_no_pool[_current_file_no])
		{
			_file_no_pool[_current_file_no]=true; 
			return _current_file_no;
		}
	}
	return -1;
}

void Master::
_release_file_no(ssize_t file_no)
{
	--_file_number;
	_file_no_pool[file_no]=false;
	return;
}

Master::IOnode_t::iterator Master::
_find_by_uri(const string &uri)
{
	IOnode_t::iterator it=_registered_IOnodes.begin();
	for(; it != _registered_IOnodes.end() && ! (it->second->uri == uri); ++it);
	return it;
}

open_file_info* Master::
_create_new_open_file_info(ssize_t 		file_no,
			   int 			flag,
			   Master_file_stat	*stat)
throw(invalid_argument, runtime_error)
{
	try
	{
		open_file_info *new_file=new open_file_info(file_no, flag, stat); 
		_buffered_files.insert(std::make_pair(file_no, new_file));
		new_file->block_size=get_block_size(stat->get_status().st_size); 
		stat->opened_file_info=new_file;

		new_file->primary_replica_node=_select_IOnode(file_no,
				MIN(NUM_OF_REPLICA, _registered_IOnodes.size()),
				new_file->IOnodes_set);
		_create_block_list(new_file->get_stat().st_size,
				new_file->block_size,
				new_file->block_list,
				new_file->IOnodes_set);

		//send open request to primary node
		_send_open_request_to_IOnodes(*new_file,
				new_file->primary_replica_node,
				new_file->block_list,
				new_file->IOnodes_set);
		//send open request to the rest of replica
		for(auto& IOnode:new_file->IOnodes_set)
		{
			_send_open_request_to_IOnodes(*new_file,
					IOnode,
					new_file->block_list,
					new_file->IOnodes_set);
		}
		return new_file;
	}
	catch(std::invalid_argument &e)
	{
		//file open error
		_release_file_no(file_no);
		throw;
	}
	catch(std::runtime_error &e)
	{
		//file open error
		_release_file_no(file_no);
		_DEBUG("%s",e.what());
		throw;
	}
}

Master_file_stat* Master::
_create_new_file_stat(const char 	*relative_path,
		      int 		exist_flag,
	       	      mode_t 		mode)
throw(std::invalid_argument)
{
	string relative_path_string 	=string(relative_path);
	string real_path		=_get_real_path(relative_path_string);
	time_t current_time;
	struct stat file_status;

	_DEBUG("file path=%s\n", real_path.c_str());
	memset(&file_status, 0, sizeof(file_status));
	time(&current_time);
	file_status.st_mtime=current_time;
	file_status.st_atime=current_time;
	file_status.st_ctime=current_time;
	file_status.st_size=0;
	file_status.st_uid=getuid();
	file_status.st_gid=getgid();
	file_status.st_mode=mode;
	file_status.st_blksize=128*KiB;
	_DEBUG("add new file states\n");
	file_stat_pool_t::iterator it=_file_stat_pool.insert(std::make_pair(relative_path_string, Master_file_stat())).first;
	Master_file_stat& new_file_stat=it->second;
	new_file_stat.full_path=it;
	new_file_stat.exist_flag=exist_flag;
	memcpy(&new_file_stat.get_status(), &file_status, sizeof(file_status));

	_create_new_backup_file(relative_path_string, mode);
	return &new_file_stat;
}

node_info* Master::
_select_IOnode(ssize_t 		file_no,
	       int 		num_of_nodes,
	       node_info_pool_t	&node_info_pool)
throw(std::runtime_error)
{
	if(0 == _registered_IOnodes.size())
	{
		//no IOnode
		throw std::runtime_error("no IOnode");
	}

	//insert main replica
	node_info* main_replica=_get_next_IOnode();
	main_replica->stored_files.insert(file_no);
	//insert sub replica
	if(static_cast<int>(_registered_IOnodes.size()-1) <num_of_nodes)
	{
		num_of_nodes=_registered_IOnodes.size()-1;
	}

	auto _replica_IOnode=_current_IOnode;
	for(;0 < num_of_nodes;num_of_nodes--)
	{
		if(end(_registered_IOnodes) == _replica_IOnode)
		{
			_replica_IOnode=begin(_registered_IOnodes);
		}
		_replica_IOnode->second->stored_files.insert(file_no);
		node_info_pool.insert((_replica_IOnode++)->second);
	}
	return main_replica; 
}

CBB::CBB_error
Master::_create_block_list(size_t 		file_size,
			   size_t 		block_size,
			   block_list_t 	&block_list,
			   node_info_pool_t 	&node_list)
{
	off64_t block_start_point	=0;
	size_t 	remaining_size		=file_size;

	//ssize_t node_id=*begin(node_list);
	if(0 == remaining_size)
	{
		block_list.insert(make_pair(0, 0));
	}
	while(0 != remaining_size)
	{
		size_t new_block_size=MIN(block_size, remaining_size);
		block_list.insert(make_pair(block_start_point, new_block_size));
		block_start_point += new_block_size;
		remaining_size -= new_block_size;
	}
	return SUCCESS;
}

CBB::CBB_error Master::
_allocate_new_blocks_for_writing(open_file_info 	&file,
				 off64_t 		start_point,
				 size_t 		size)
{
	off64_t current_point=get_block_start_point(start_point, size);
	block_list_t::iterator current_block=
		file.block_list.find(current_point); ssize_t remaining_size=size;
	if(end(file.block_list) != current_block)
	{
		if(size + start_point < current_block->first + current_block->second)
		{
			//no need to append new block
			return SUCCESS;
		}
	}

	for(;remaining_size>0;remaining_size-=BLOCK_SIZE, current_point+=BLOCK_SIZE)
	{
		size_t block_size=MIN(remaining_size, static_cast<ssize_t>(BLOCK_SIZE));
		if(end(file.block_list) == current_block)
		{
			_append_block(file, current_point, block_size);
			current_block=end(file.block_list);
		}
		else
		{
			current_block->second=MAX(block_size, current_block->second);
			current_block++;
		}
	}
	return SUCCESS;
}

/*int Master::_allocate_one_block(const struct open_file_info& file,
		node_info_pool_t& node_info_pool)throw(std::bad_alloc)
{
	//temp solution
	int node_id=*file.nodes.begin();
	for(auto nodes:file.nodes)
	{
		node_info_pool.insert(nodes);
	}
	//node_info &selected_node=_registered_IOnodes.at(node_id);
	return;
	if(selected_node.avaliable_memory >= BLOCK_SIZE)
	{
		selected_node.avaliable_memory -= BLOCK_SIZE;
		return node_id;
	}
	else
	{
		throw std::bad_alloc();
	}
}*/

void Master::
_append_block(struct open_file_info& file,
	      off64_t 		     start_point,
	      size_t 		     size)
{
	_DEBUG("append block=%ld\n", start_point);
	file.block_list.insert(std::make_pair(start_point, size));
	//file.IOnodes_set.insert(node_id);
}

CBB::CBB_error Master::
_remove_open_file(ssize_t file_no)
{
	File_t::iterator it=_buffered_files.find(file_no);
	if(_buffered_files.end() != it)
	{
		open_file_info* file=it->second;
		_remove_open_file(file);
	}
	return SUCCESS;
}

CBB::CBB_error Master::
_remove_open_file(open_file_info* file_info)
{
	ssize_t file_no=file_info->file_no;

	_send_remove_request(file_info->primary_replica_node, file_no);
	//remove file_no from stored files list
	file_info->primary_replica_node->stored_files.erase(file_no);

	for(auto IOnode:file_info->IOnodes_set)
	{
		_send_remove_request(IOnode, file_no);
		//remove file_no from stored files list
		IOnode->stored_files.erase(file_no);
	}

	_buffered_files.erase(file_no);
	delete file_info;
	_release_file_no(file_no);
	return SUCCESS;
}

void Master::
_send_remove_request(node_info 	*IOnode,
		     ssize_t 	file_no)
{
	extended_IO_task* output=allocate_output_task(_get_my_thread_id());
	output->set_handle(IOnode->handle);
	output->push_back(UNLINK);
	_DEBUG("unlink file no=%ld\n", file_no);
	output->push_back(file_no);
	output_task_enqueue(output);
}

//low performance
CBB::CBB_error Master::
_get_file_stat_from_dir(const string& dir,
			dir_t&	      files)
{
	ssize_t start_pos=dir.size();
	if(dir[start_pos-1] != '/')
	{
		start_pos++;
	}
	//_file_stat_pool.rd_lock();
	file_stat_pool_t::const_iterator it=_file_stat_pool.lower_bound(dir), end=_file_stat_pool.end();
	if(end!= it && it->first == dir)
	{
		++it;
	}
	while(end != it)
	{
		const std::string& file_name=(it++)->first;
		if( 0 == file_name.compare(0, dir.size(), dir))
		{
			size_t end_pos=file_name.find('/', start_pos);
			if(std::string::npos == end_pos)
			{
				end_pos=file_name.size();
			}
			files.insert(file_name.substr(start_pos, end_pos-start_pos));
		}
		else
		{
			break;
		}
	}
	//_file_stat_pool.unlock();
	return SUCCESS;
}

CBB::CBB_error Master::
_buffer_all_meta_data_from_remote(const char* mount_point)
throw(CBB_configure_error)
{
	char file_path[PATH_MAX];
	DIR *dir=opendir(mount_point);
	if(nullptr == dir)
	{
		_DEBUG("file path%s\n", mount_point);
		perror("open remote storage");
		throw CBB_configure_error("open remote storage");
	}
	strcpy(file_path, mount_point);

	_LOG("start to buffer file status from remote\n");
	struct stat file_status;
	lstat(file_path, &file_status);
	file_stat_pool_t::iterator it=
		_file_stat_pool.insert(
				std::make_pair("/", Master_file_stat(file_status, "/", EXISTING))).first;
	it->second.set_file_full_path(it);
	size_t len=strlen(mount_point);
	if('/' != file_path[len])
	{
		file_path[len++]='/';
	}
	int ret=_dfs_items_in_remote(dir, file_path, file_path+len-1, len);
	closedir(dir);
	_LOG("buffering file status finished\n");
	return ret;
}

CBB::CBB_error Master::
_dfs_items_in_remote(DIR 	*current_remote_directory,
		     char	*file_path,
		     const char	*file_relative_path,
		     size_t 	offset)
throw(std::runtime_error)
{
	const struct dirent* dir_item=nullptr;
	while(nullptr != (dir_item=readdir(current_remote_directory)))
	{
		if(0 == strcmp(dir_item->d_name, ".") || 0 == strcmp(dir_item->d_name, ".."))
		{
			continue;
		}
		strcpy(file_path+offset, dir_item->d_name);
		size_t len=strlen(dir_item->d_name);
		file_path[offset+len]=0;
		if(master_number == file_path_hash(std::string(file_relative_path), master_total_size))
		{
			struct stat file_status;
			lstat(file_path, &file_status);
			file_stat_pool_t::iterator it=
				_file_stat_pool.insert(std::make_pair(
					file_relative_path, Master_file_stat(
						file_status, dir_item->d_name, EXISTING))).first;
			it->second.set_file_full_path(it);
		}
		if(DT_DIR == dir_item->d_type)
		{
			DIR* new_dir=opendir(file_path);
			if(nullptr == new_dir)
			{
				throw std::runtime_error(file_path);
			}
			file_path[offset+len]='/';
			_dfs_items_in_remote(new_dir, file_path, file_relative_path, offset+len+1);
			closedir(new_dir);
		}
	}
	return SUCCESS;
}

CBB::CBB_error Master::
get_IOnode_handle_map(handle_map_t& handle_map)
{
	for(const auto& IOnode:_IOnode_handle)
	{
		handle_map.insert(std::make_pair(IOnode.first, IOnode.second->node_id));
	}
	return SUCCESS;
}

CBB::CBB_error Master::
node_failure_handler(extended_IO_task* new_task)
{
	//dummy code
	_DEBUG("IOnode failed from task handle=%p\n", new_task->get_handle());
	return send_input_for_handle_error(new_task->get_handle());
}

CBB::CBB_error Master::
node_failure_handler(comm_handle_t handle)
{
	//dummy code
	_DEBUG("IOnode failed handle=%p\n", handle);
	return send_input_for_handle_error(handle);
}

CBB::CBB_error Master::
_remote_rename(Common::remote_task* new_task)
{
	string 	*old_name	=static_cast<string*>(new_task->get_task_data());
	string	*new_name	=static_cast<string*>(new_task->get_extended_task_data());
	string 	old_real_path	=_get_real_path(*old_name);
	string	new_real_path	=_get_real_path(*new_name);
	int 	ret		=SUCCESS;

	_LOG("rename %s to %s\n", old_real_path.c_str(), new_real_path.c_str());
	if(0 != (ret=rename(old_real_path.c_str(), new_real_path.c_str())))
	{
		perror("rename");
	}
	delete old_name;
	return ret;
}

CBB::CBB_error Master::_remote_rmdir(Common::remote_task* new_task)
{
	int 	ret		=SUCCESS;
	string 	*dir_name	=static_cast<string*>(new_task->get_task_data());
	string 	real_dir_name	=_get_real_path(*dir_name);

	_LOG("rmdir %s\n", real_dir_name.c_str());
	if(0 != (ret=rmdir(real_dir_name.c_str())))
	{
		perror("rmdir");
	}

	return ret;
}

CBB::CBB_error Master::_remote_unlink(Common::remote_task* new_task)
{
	int 	ret		=SUCCESS;
	string 	*file_name	=static_cast<string*>(new_task->get_task_data());
	string 	real_file_name	=_get_real_path(*file_name);

	_LOG("unlink file %s\n", real_file_name.c_str());
	if(0 != (ret=unlink(real_file_name.c_str())))
	{
		perror("unlink");
	}

	return ret;
}

CBB::CBB_error Master::_remote_mkdir(Common::remote_task* new_task)
{
	int 	ret		=SUCCESS;
	string 	*dir_name	=static_cast<string*>(new_task->get_task_data());
	mode_t 	*mode		=static_cast<mode_t*>(new_task->get_extended_task_data());
	string 	real_dir_name	=_get_real_path(*dir_name);

	_LOG("mkdir %s\n", real_dir_name.c_str());
	if(0 != (ret=mkdir(real_dir_name.c_str(), *mode)))
	{
		perror("mkdir");
	}

	return ret;
}

node_info* Master::_get_next_IOnode()
{
	if(0 == _registered_IOnodes.size())
	{
		//out of nodes
		return nullptr;
	}
	if(end(_registered_IOnodes) == _current_IOnode)
	{
		_current_IOnode=begin(_registered_IOnodes);
	}
	return (_current_IOnode++)->second;
}

CBB::CBB_error Master::_parse_IOnode_failure(extended_IO_task* new_task)
{
	ssize_t IOnode_id=0;
	new_task->pop(IOnode_id);
	_DEBUG("IOnode failure node_id=%ld\n", IOnode_id);
	
	IOnode_t::iterator it=_registered_IOnodes.find(IOnode_id);

	if(end(_registered_IOnodes) != it)
	{
		_IOnode_failure_handler(it->second);
	}
	return SUCCESS;
}

/*void Master::flush_file_stat()
{
	for(auto& file_stat:_file_stat_pool)
	{
		_DEBUG("file path %s, address %p\n", file_stat.first.c_str(), &file_stat);
	}
}*/

CBB::CBB_error Master::
_create_new_backup_file(
		const string 	&path,
	   	mode_t		mode)
{
	std::string file_path=_get_backup_path(path);
	_DEBUG("create new backup file %s\n", file_path.c_str());

	if(S_IFREG & mode)
	{
		int fd=creat(file_path.c_str(), mode);
		if( -1 == fd)
		{
			perror("create backup file");
			return FAILURE;
		}
		else
		{
			close(fd);
			return SUCCESS;
		}
	}
	else if(S_IFDIR & mode)
	{
		int ret=mkdir(file_path.c_str(), mode);
		if( -1 == ret)
		{
			perror("create backup dir");
			return FAILURE;
		}
		else
		{
			return SUCCESS;
		}
	}
	return FAILURE;
}

CBB::CBB_error
Master::_update_backup_file_size(const string 	&path,
				 size_t		size)
{
	std::string file_path=_get_backup_path(path);
	_DEBUG("update backup file %s size %lu\n", file_path.c_str(), size);
	
	int ret=truncate(file_path.c_str(), size);
	if(-1 == ret)
	{
		perror("update backup file");
		return FAILURE;
	}
	else
	{
		return SUCCESS;
	}
}

CBB::CBB_error Master::
_set_master_number_size(const char*	master_ip_list,
			int&		master_number,
			int&		master_total_size)
{
	//struct hostent* ip_list_ent	=get_my_ip_list();	
	struct ifaddrs* interface_array =nullptr;
	char		ip[200];
	struct in_addr 	master_addr;
	bool set=false;

	
	master_total_size=0;
	//only ipv4 now
	if(0 != getifaddrs(&interface_array))
	{
		return FAILURE;
	}

	while(nullptr != (master_ip_list = parse_master_config_ip(master_ip_list, ip)))
	{
		if(0 == inet_pton(AF_INET, ip, &master_addr))
		{
			return FAILURE;
		}
		for(struct ifaddrs* interface=interface_array;
				nullptr != interface; interface=interface->ifa_next)
		{
			if(AF_INET == interface->ifa_addr->sa_family)
				//	|| AF_ROUTE == interface->ifa_addr->sa_family)
			{
				struct in_addr* temp_attr=nullptr;
				temp_attr = &((struct sockaddr_in *)interface->ifa_addr)->sin_addr;
				if(temp_attr->s_addr == master_addr.s_addr)
				{
					master_number = master_total_size;
					Server::_set_uri(inet_ntoa(*temp_attr));
					set=true;
				}

			}
			/*else
			{
				temp_attr = &((struct sockaddr_in6 *)interface->ifa_addr)->sin6_addr;
			}*/
		}
		master_total_size++;
	}

	freeifaddrs(interface_array);
	if(set)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

void Master::configure_dump()
{
	_LOG("master number=%d\n", this->master_number);
	_LOG("master mount point=%s\n",this->_mount_point.c_str());
	_LOG("master metadata backup point=%s\n",metadata_backup_point.c_str());
	_LOG("master total size=%d\n", this->master_total_size);
	_LOG("my uri=%s\n", Server::_get_my_uri());
	return;
}

node_info* Master::
_get_node_info_from_handle(comm_handle_t handle)
{
	for(auto IOnode:_IOnode_handle)
	{
		if(compare_handle(IOnode.first, handle))
		{
			return IOnode.second;
		}
	}
	return nullptr;
}
