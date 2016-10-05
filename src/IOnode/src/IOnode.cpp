/*
 * IOnode.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
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

#include "IOnode.h"
#include "CBB_const.h"

using namespace CBB::IOnode;
using namespace CBB::Common;
using namespace std;

const char *IOnode::IONODE_MOUNT_POINT="CBB_IONODE_MOUNT_POINT";

IOnode::block::block(off64_t	 start_point,
		     size_t	 data_size,
		     bool 	 dirty_flag,
		     bool 	 valid,
		     file* 	 file_stat) throw(std::bad_alloc):
	data_size(data_size),
	data(nullptr),
	start_point(start_point),
	dirty_flag(dirty_flag),
	valid(INVALID),
	file_stat(file_stat),
	write_back_task(nullptr),
	locker(),
	TO_BE_DELETED(CLEAN)
{
	if(INVALID != valid)
	{
		data=malloc(BLOCK_SIZE);
		if( nullptr == data)
		{
		   throw std::bad_alloc();
		}
	}
	return;
}

IOnode::block::~block()
{
	free(data);
}

IOnode::block::block(const block & src):
	data_size(src.data_size),
	data(src.data), 
	start_point(src.start_point),
	dirty_flag(src.dirty_flag),
	valid(src.valid),
	file_stat(nullptr),
	write_back_task(nullptr),
	locker(),
	TO_BE_DELETED(CLEAN)
{};

IOnode::file::file(const char	 *path,
		   int		 exist_flag,
		   ssize_t file_no):
	file_path(path),
	exist_flag(exist_flag),
	dirty_flag(CLEAN),
	main_flag(SUB_REPLICA),
	file_no(file_no),
	blocks(),
	IOnode_pool()
{}

IOnode::file::~file()
{
	block_info_t::const_iterator end=blocks.end();
	for(block_info_t::iterator block_it=blocks.begin();
			block_it!=end;++block_it)
	{
		delete block_it->second;
	}
}

void IOnode::block::allocate_memory()throw(std::bad_alloc)
{
	if(nullptr != data)
	{
		free(data);
	}
	data=malloc(BLOCK_SIZE);
	if(nullptr == data)
	{
		throw std::bad_alloc();
	}
	valid=VALID;
	return;
}

IOnode::IOnode(const std::string&	master_ip,
	       int			master_port) throw(std::runtime_error):
	Server(IONODE_QUEUE_NUM, IONODE_PORT), 
	CBB_data_sync(),
	my_node_id(-1),
	_files(file_t()),
	_current_block_number(0), 
	_MAX_BLOCK_NUMBER(MAX_BLOCK_NUMBER), 
	_memory(MEMORY), 
	my_uri(),
	master_uri(),
	master_handle(nullptr),
	my_handle(nullptr),
	_mount_point(std::string()),
	IOnode_handle_pool()
{
	const char *IOnode_mount_point=getenv(IONODE_MOUNT_POINT);
	master_uri=master_ip;

	if( nullptr == IOnode_mount_point)
	{
		throw std::runtime_error("please set IONODE_MOUNT_POINT environment value");
	}

	_setup_queues();

	try
	{
		Server::_init_server();
	}
	catch(std::runtime_error& e)
	{
		throw;
	}
	_mount_point=std::string(IOnode_mount_point);
}

int IOnode::_setup_queues()
{
	CBB_data_sync::set_queues(&_communication_input_queue.at(DATA_SYNC_QUEUE_NUM),
			  	  &_communication_output_queue.at(DATA_SYNC_QUEUE_NUM));
	return SUCCESS;
}

IOnode::~IOnode()
{
	_DEBUG("I am going to shut down\n");
	_unregister();
}

int IOnode::start_server()
{
	Server::start_server();
	CBB_data_sync::start_listening();
	if(-1  ==  (my_node_id=_register(&_communication_input_queue, &_communication_output_queue)))
	{
		throw std::runtime_error("Get Node Id Error"); 
	}
	while(1)
	{
		sleep(1000000);
	}
	return SUCCESS;
}

int IOnode::_unregister()
{
	/*extended_IO_task* output=output_queue->allocate_tmp_node();
	output->set_handle(_master_handle);
	output->push_back(CLOSE_CLIENT);
	output->queue_enqueue();*/
	return SUCCESS;
}

ssize_t IOnode::_register(communication_queue_array_t* input_queue,
			communication_queue_array_t* output_queue) throw(std::runtime_error)
{ 
	char *uri=nullptr;
	_LOG("start registration\n");
	try
	{
		master_handle=connect_to_server(master_uri.c_str(), MASTER_PORT);
		Server::_add_handle(master_handle, EPOLLPRI);
	}
	catch(std::runtime_error &e)
	{
		throw;
	}
	get_uri_from_handle(master_handle, &(uri));
	this->my_uri=string(uri);
	_LOG("uri=%s\n",uri);

	extended_IO_task* output=allocate_output_task(SERVER_THREAD_NUM);
	output->set_handle(master_handle);
	output->push_back(REGISTER);
	output->push_back_string(my_uri);
	output->push_back(_memory);
	output_task_enqueue(output);

	extended_IO_task* response=get_communication_input_queue(SERVER_THREAD_NUM)->get_task();
	ssize_t id=-1;
	response->pop(id);
	get_communication_input_queue(SERVER_THREAD_NUM)->task_dequeue();
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
int IOnode::_parse_request(extended_IO_task* new_task)
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
		_DEBUG("unrecognized command\n"); break; 
	}
	return ans; 
}

int IOnode::_promoted_to_primary(extended_IO_task* new_task)
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
		_get_replica_node_info(new_task, file_info);

		add_data_sync_task(DATA_SYNC_INIT, &file_info, 0, 0, -1, 0, IOnode_handle_pool.at(new_node_id));
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

int IOnode::_replace_replica(extended_IO_task* new_task)
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

int IOnode::_remove_IOnode(extended_IO_task* new_task)
{
	ssize_t node_id	=0;

	new_task->pop(node_id);
	node_handle_pool_t::iterator it=IOnode_handle_pool.find(node_id);
	_LOG("remove IOnode id=%ld\n", node_id);

	if(end(IOnode_handle_pool) != it)
	{
		Close(it->second);
		IOnode_handle_pool.erase(it);
	}
	return SUCCESS;
}

int IOnode::_register_new_IOnode(extended_IO_task* new_task)
{
	ssize_t new_IOnode_id	=0;
	comm_handle_t handle	=new_task->get_handle();

	new_task->pop(new_IOnode_id);
	_LOG("register new IOnode %ld \n", new_IOnode_id);

	IOnode_handle_pool.insert(std::make_pair(new_IOnode_id, handle));
	return SUCCESS;
}

int IOnode::_send_data(extended_IO_task* new_task)
{
	ssize_t file_no		=0;
	off64_t start_point	=0;
	off64_t offset		=0;
	size_t  size		=0;
	ssize_t remaining_size	=0;
	int 	ret		=SUCCESS;

	new_task->pop(file_no);
	new_task->pop(start_point);
	new_task->pop(offset);
	new_task->pop(size);
	_LOG("request for send data\n");
	_DEBUG("file_no=%ld, start_point=%ld, offset=%ld, size=%lu\n", file_no, start_point, offset, size);

	extended_IO_task* output=init_response_task(new_task);
	try
	{
		file& _file=_files.at(file_no);
		const std::string& path = _file.file_path;
		remaining_size=size;
		size_t IO_size=offset+size>BLOCK_SIZE?BLOCK_SIZE-offset:size;
		_DEBUG("file path=%s\n", path.c_str());

		output->push_back(SUCCESS);
		while(remaining_size > 0)
		{
			_DEBUG("remaining_size=%ld, start_point=%ld\n", remaining_size, start_point);
			block* requested_block=nullptr;
			try{
				requested_block=_file.blocks.at(start_point);
			}catch(std::out_of_range&e )
			{
				_DEBUG("out of range\n");
				break;
			}
			requested_block->file_stat=&_file;
			if(INVALID == requested_block->valid)
			{
				requested_block->allocate_memory();
				if(EXISTING == _file.exist_flag)
				{
					_read_from_storage(path, requested_block);
				}
			}
			start_point+=BLOCK_SIZE;
			output->push_send_buffer(reinterpret_cast<char*>(requested_block->data)+offset, IO_size);
			remaining_size-=IO_size;
			IO_size=MIN(remaining_size, static_cast<ssize_t>(BLOCK_SIZE));
			offset=0;
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
	return ret;

}

int IOnode::_receive_data(extended_IO_task* new_task)
{
	ssize_t file_no		=0;
	off64_t start_point	=0;
	off64_t offset		=0;
	size_t 	size		=0;
	ssize_t remaining_size	=0;
	int 	ret		=SUCCESS;

	new_task->pop(file_no);
	new_task->pop(start_point);
	new_task->pop(offset);
	new_task->pop(size);
	_LOG("request for receive data\n");
	_DEBUG("file_no=%ld, start_point=%ld, offset=%ld, size=%lu\n", file_no, start_point, offset, size);

	try
	{
		remaining_size=size;
		file& _file=_files.at(file_no);
		block_info_t &blocks=_file.blocks;
		size_t receive_size=(offset + size) > BLOCK_SIZE?BLOCK_SIZE-offset:size;
		off64_t tmp_offset=offset;

		while(0 < remaining_size)
		{
			_DEBUG("receive_data start point %ld, offset %ld, receive size %ld, remaining size %ld\n", start_point, tmp_offset, receive_size, remaining_size);
			ssize_t IOsize=update_block_data(blocks, _file, start_point, tmp_offset, receive_size, new_task);
			remaining_size	-=IOsize;
			receive_size	=MIN(remaining_size, static_cast<ssize_t>(BLOCK_SIZE));
			start_point	+=IOsize+tmp_offset;
			tmp_offset	=0;
		}
		_file.dirty_flag=DIRTY;
		_sync_data(_file, start_point, offset, size, new_task->get_handle());
		ret=SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		extended_IO_task* output=init_response_task(new_task);
		output->push_back(FILE_NOT_FOUND);
		ret=FAILURE;
		output_task_enqueue(output);
	}
	return ret;
}

size_t IOnode::update_block_data(block_info_t&		blocks,
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
		_block=blocks.insert(std::make_pair(start_point, new block(start_point, size, CLEAN, INVALID, &file))).first->second;
	}
	if(INVALID == _block->valid)
	{
		_block->allocate_memory();
		if(EXISTING == file.exist_flag)
		{
			_block->data_size=_read_from_storage(file.file_path, _block);
		}
		_block->valid = VALID;
	}
	if(_block->data_size < offset+size)
	{
		_block->data_size = offset+size;
	}

	ret=new_task->get_received_data(static_cast<unsigned char*>(_block->data)+offset, size);
	_block->dirty_flag=DIRTY;
	return ret;
}

int IOnode::_sync_data(file& 	 file_info, 
		       off64_t	 start_point, 
		       off64_t	 offset,
		       ssize_t	 size,
		       comm_handle_t handle)
{
	_DEBUG("offset %ld\n", offset);
	return add_data_sync_task(DATA_SYNC_WRITE, &file_info, start_point, offset, size, 0, handle);
}

int IOnode::_open_file(extended_IO_task* new_task)
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
	_DEBUG("openfile fileno=%ld, path=%s\n", file_no, path_buffer);
	
	file& _file=_files.insert(std::make_pair(file_no, file(path_buffer, exist_flag, file_no))).first->second;
	//get blocks
	new_task->pop(count);
	for(int i=0;i<count;++i)
	{
		_append_block(new_task, _file);
	}
	//get replica ips
	_get_replica_node_info(new_task, _file);

	return SUCCESS; 
}

int IOnode::_get_replica_node_info(extended_IO_task* 	new_task,
			       	   file& 		_file)
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

int IOnode::_get_IOnode_info(extended_IO_task*	new_task, 
			     file& 		_file)
{
	comm_handle_t new_handle=nullptr;
	char* 	node_ip		=nullptr;
	ssize_t node_id		=0;

	new_task->pop(node_id);
	new_task->pop_string(&node_ip);
	node_handle_pool_t::const_iterator it;

	_DEBUG("try to connect to %s\n", node_ip);
	
	if(end(IOnode_handle_pool) == (it=IOnode_handle_pool.find(node_id)))
	{
		new_handle=_connect_to_new_IOnode(node_id, my_node_id, node_ip);
	}
	else
	{
		new_handle=it->second;
	}
	_file.IOnode_pool.insert(std::make_pair(node_id, new_handle));
	return SUCCESS;
}

comm_handle_t IOnode::
_connect_to_new_IOnode(ssize_t 	destination_node_id,
		       ssize_t 	my_node_id,
		       const char* 	node_ip)
{
	comm_handle_t handle=connect_to_server(node_ip, IONODE_PORT); 
	_DEBUG("connect to destination node id %ld\n", destination_node_id);

	Server::_add_handle(handle);
	extended_IO_task* output=allocate_output_task(0);
	output->set_handle(handle);
	output->push_back(NEW_IONODE);
	output->push_back(my_node_id);
	output_task_enqueue(output);
	IOnode_handle_pool.insert(std::make_pair(destination_node_id, handle));
	return handle;
}

int IOnode::_close_file(extended_IO_task* new_task)
{
	ssize_t file_no=0;
	new_task->pop(file_no);
	try
	{
		file& _file=_files.at(file_no);
		block_info_t &blocks=_file.blocks;
		_DEBUG("close file, path=%s\n", _file.file_path.c_str());
		for(block_info_t::iterator it=blocks.begin();
				it != blocks.end();++it)
		{
			block* _block=it->second;
			_block->lock();

			if((DIRTY == _block->dirty_flag 	&&
			  nullptr == _block->write_back_task) 	||
			  (CLEAN == _block->dirty_flag && nullptr != _block->write_back_task))
			{
				//_write_to_storage(path, _block);
				_block->write_back_task=CBB_remote_task::add_remote_task(CBB_REMOTE_WRITE_BACK, _block);
				_DEBUG("add write back\n");
			}
			_block->unlock();
		}
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		return FAILURE;
	}
}

int IOnode::_rename(extended_IO_task* new_task)
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

void IOnode::_append_block(extended_IO_task* 	new_task,
			   file& 		file_stat)throw(std::runtime_error)
{
	off64_t start_point	=0;
	size_t 	data_size	=0;
	block*	new_block	=nullptr;

	new_task->pop(start_point); 
	new_task->pop(data_size); 
	_DEBUG("append request from Master\n");
	_DEBUG("start_point=%lu, data_size=%lu\n", start_point, data_size);
	if(nullptr == (new_block=new block(start_point,
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

int IOnode::_append_new_block(extended_IO_task* new_task)
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
			blocks.insert(std::make_pair(start_point, new block(start_point, data_size, CLEAN, INVALID, &_file)));
		}
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		return FAILURE;
	}
}

size_t IOnode::_read_from_storage(const std::string& 	path,
				  block* 		block_data)
				  throw(std::runtime_error)
{
	off64_t 	start_point	=block_data->start_point;
	std::string 	real_path	=_get_real_path(path);
	int 		fd		=open64(real_path.c_str(), O_RDONLY); 
	ssize_t		ret		=SUCCESS; 
	struct iovec 	vec;
	char 		*buffer		=reinterpret_cast<char*>(block_data->data);
	size_t 		size		=block_data->data_size;
	_DEBUG("%s\n", real_path.c_str());

	if(-1 == fd)
	{
		perror("File Open Error"); 
		return 0;
	}
	if(-1  == lseek(fd, start_point, SEEK_SET))
	{
		perror("Seek"); 
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
	close(fd);
	return block_data->data_size-size;
}

size_t IOnode::_write_to_storage(block* block_data)throw(std::runtime_error)
{
	block_data->lock();
	block_data->dirty_flag=CLEAN;

	std::string real_path	=_get_real_path(block_data->file_stat->file_path);
	off64_t     start_point	=block_data->start_point;
	size_t	    size	=block_data->data_size, len=size;
	const char* buf		=static_cast<const char*>(block_data->data);
	ssize_t	    ret		=0;

	block_data->unlock();
	int fd = open64(real_path.c_str(),O_WRONLY|O_CREAT, 0600);
	if( -1 == fd)
	{
		perror("Open File");
		throw std::runtime_error("Open File Error\n");
	}
	off64_t pos;
	if(-1 == (pos=lseek64(fd, start_point, SEEK_SET)))
	{
		perror("Seek"); 
		throw std::runtime_error("Seek File Error"); 
	}
	_DEBUG("write to %s, size=%ld, offset=%ld\n", real_path.c_str(), len, start_point);
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
	close(fd);
	block_data->lock();
	block_data->write_back_task=nullptr;
	block_data->unlock();
	if(block_data->TO_BE_DELETED)
	{
		delete block_data;
	}
	return size-len;
}

int IOnode::_flush_file(extended_IO_task* new_task)
{
	ssize_t file_no=0;
	new_task->pop(file_no);
	try
	{
		file& _file=_files.at(file_no);
		_DEBUG("flush file file_no=%ld, path=%s\n", file_no, _file.file_path.c_str());
		block_info_t &blocks=_file.blocks;
		if(CLEAN == _file.dirty_flag)
		{
			return SUCCESS;
		}
		for(block_info_t::iterator it=blocks.begin();
				it != blocks.end();++it)
		{
			block* _block=it->second;
			_block->lock();
			if((DIRTY == _block->dirty_flag && nullptr == _block->write_back_task) ||
					(CLEAN == _block->dirty_flag && nullptr != _block->write_back_task))
			{
				//_write_to_storage(path, _block);
				_block->write_back_task=CBB_remote_task::add_remote_task(CBB_REMOTE_WRITE_BACK, _block);
				_DEBUG("add write back\n");
			}
			_block->unlock();
		}
		return SUCCESS;
	}
	catch(std::out_of_range& e)
	{
		return FAILURE;
	}
}

int IOnode::_register_new_client(extended_IO_task* new_task)
{

	//getpeername(new_task->get_handle(), reinterpret_cast<sockaddr*>(&addr), &len); 
	//_LOG("new client sockfd=%d ip=%s\n", new_task->get_handle(), inet_ntoa(addr.sin_addr));
	extended_IO_task* output=init_response_task(new_task);
	output->push_back(SUCCESS);
	output_task_enqueue(output);
	return SUCCESS;
}

int IOnode::_close_client(extended_IO_task* new_task)
{
	_LOG("close client\n");
	comm_handle_t handle=new_task->get_handle();
	Server::_delete_handle(handle);
	Close(handle);
	return SUCCESS;
}

int IOnode::_unlink(extended_IO_task* new_task)
{
	ssize_t file_no	=0;
	int	ret	=0;
	_LOG("unlink\n");
	new_task->pop(file_no);
	_LOG("file no=%ld\n", file_no);
	ret=_remove_file(file_no);	
	return ret;
}

inline std::string IOnode::_get_real_path(const char* path)const
{
	return _mount_point+std::string(path);
}

inline std::string IOnode::_get_real_path(const std::string& path)const
{
	return _mount_point+path;
}

int IOnode::_truncate_file(extended_IO_task* new_task)
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

int IOnode::_remove_file(ssize_t file_no)
{
	file_t::iterator it=_files.find(file_no);
	if(_files.end() != it)
	{
		for(block_info_t::iterator block_it=it->second.blocks.begin();
				block_it!=it->second.blocks.end();++block_it)
		{
			block_it->second->lock();
			if(nullptr != block_it->second->write_back_task)
			{
				if(CLEAN == block_it->second->dirty_flag)
				{
					block_it->second->TO_BE_DELETED=SET;
				}
				else
				{
					block_it->second->write_back_task->set_task_data(nullptr);
				}
			}
			block_it->second->unlock();
		}
		_files.erase(it);
	}
	return SUCCESS;
}

int IOnode::remote_task_handler(remote_task* new_task)
{
	block* IO_block=static_cast<block*>(new_task->get_task_data());
	if(nullptr != IO_block)
	{
		switch(new_task->get_task_id())
		{
			case CBB_REMOTE_WRITE_BACK:
#ifdef WRITE_BACK
				_DEBUG("write back\n");
				_write_to_storage(IO_block);
#endif
				break;
		}
	}
	return SUCCESS;
}

int IOnode::_heart_beat(extended_IO_task* new_task)
{
	int ret=0;
	new_task->pop(ret);
	return ret;
}

int IOnode::_node_failure(extended_IO_task* new_task)
{
	comm_handle_t handle=new_task->get_handle();
	_DEBUG("node failure, close handle %p\n", handle);
	Close(handle);

	if(handle->socket == master_handle->socket)
	{
		_DEBUG("Master failed !!!\n");
	}
	else
	{
		//remove handle if failed node if IOnode
		for(auto& node:IOnode_handle_pool)
		{
			if(handle == node.second)
			{
				IOnode_handle_pool.erase(node.first);
				break;
			}
		}
	}
	return SUCCESS;
}

int IOnode::data_sync_parser(data_sync_task* new_task)
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

int IOnode::_sync_init_data(data_sync_task* new_task)
{
	file* requested_file=static_cast<file*>(new_task->_file);
	_LOG("send sync data file_no = %ld\n", requested_file->file_no);

	//only sync dirty data
	if(DIRTY == requested_file->dirty_flag)
	{
		_send_sync_data(new_task->handle, requested_file, 0, 0, MAX_FILE_SIZE);
	}

	return SUCCESS;
}

int IOnode::_sync_write_data(data_sync_task* new_task)
{
	file* 		requested_file	=static_cast<file*>(new_task->_file);
	comm_handle_t	handle		=new_task->handle;
	int 		receiver_id	=new_task->receiver_id;
	_LOG("send sync data file_no = %ld\n", requested_file->file_no);

#ifdef STRICT_SYNC_DATA
	for(auto& replicas:requested_file->IOnode_pool)
	{
		_send_sync_data(replicas.second, requested_file, new_task->start_point, new_task->offset, new_task->size);
	}
#endif
	_DEBUG("send response to handle %p\n", handle);
	extended_IO_task* output_task=allocate_data_sync_task();
	output_task->set_handle(handle);
	output_task->set_receiver_id(receiver_id);
	output_task->push_back(SUCCESS);
	data_sync_task_enqueue(output_task);
#ifndef STRICT_SYNC_DATA
	for(auto& replicas:requested_file->IOnode_pool)
	{
		_send_sync_data(replicas.second, requested_file, new_task->start_point, new_task->offset, new_task->size);
	}
#endif
#ifdef SYNC_DATA_WITH_REPLY
	for(auto& replica:requested_file->IOnode_pool)
	{
		_get_sync_response();
	}
#endif
	return SUCCESS;
}

int IOnode::_get_sync_response()
{
	int ret=SUCCESS;

	extended_IO_task* response=get_data_sync_response();
	response->pop(ret);
	data_sync_response_dequeue(response);
	return ret;
}

int IOnode::_send_sync_data(comm_handle_t handle,
			    file* 	requested_file,
			    off64_t 	start_point,
			    off64_t	offset,
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
		output_task->push_send_buffer(static_cast<char*>(requested_block->data), requested_block->data_size);
		size-=requested_block->data_size;
		ret_size+=requested_block->data_size;
		block_it++;
	}
	output_task->push_back(ret_size);
	_DEBUG("data size=%ld\n", ret_size);
	data_sync_task_enqueue(output_task);
	return SUCCESS;
}

int IOnode::_get_sync_data(extended_IO_task* new_task)
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
			size-=update_block_data(blocks, _file, start_point, 0, IO_size, new_task);
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

void IOnode::configure_dump()
{
	_LOG("under master %s\n", master_uri.c_str());
	_LOG("mount point=%s\n", _mount_point.c_str());
	_LOG("max block %d\n", _MAX_BLOCK_NUMBER);
	_LOG("avaliable memory %ld\n", _memory); //remain available memory; 
	_LOG("my uri=%s\n", my_uri.c_str());
}
