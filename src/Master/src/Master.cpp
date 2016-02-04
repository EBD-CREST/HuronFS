/*OB`
 * Master.cpp * *  Created on: Aug 8, 2014 *      Author: xtq
 */
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sched.h>

#include "Master.h"
#include "CBB_internal.h"
#include "Communication.h"

using namespace CBB::Common;
using namespace CBB::Master;
using namespace std;

const char *Master::MASTER_MOUNT_POINT="CBB_MASTER_MOUNT_POINT";
const char *Master::MASTER_NUMBER="CBB_MASTER_MY_ID";
const char *Master::MASTER_TOTAL_NUMBER="CBB_MASTER_TOTAL_NUMBER";

Master::Master()throw(std::runtime_error):
	Server(MASTER_QUEUE_NUM, MASTER_PORT), 
	CBB_heart_beat(),
	_registed_IOnodes(IOnode_t()), 
	_file_stat_pool(),
	_buffered_files(), 
	_IOnode_socket(IOnode_sock_t()), 
	_file_number(0), 
	_node_id_pool(new bool[MAX_NODE_NUMBER]), 
	_file_no_pool(new bool[MAX_FILE_NUMBER]), 
	_current_node_number(0), 
	_current_file_no(0),
	_mount_point(),
	_current_IOnode(_registed_IOnodes.end())
{
	memset(_node_id_pool, 0, MAX_NODE_NUMBER*sizeof(bool)); 
	memset(_file_no_pool, 0, MAX_FILE_NUMBER*sizeof(bool)); 
	const char *master_mount_point=getenv(MASTER_MOUNT_POINT);
	const char *master_number_string = getenv(MASTER_NUMBER);
	const char *master_total_size_string= getenv(MASTER_TOTAL_NUMBER);
	
	if(nullptr == master_mount_point)
	{
		throw std::runtime_error("please set master mount point");
	}
	if(nullptr == master_number_string)
	{
		throw std::runtime_error("please set master number");
	}
	if(nullptr == master_total_size_string)
	{
		throw std::runtime_error("please set master total number");
	}

	_setup_queues();

	master_number = atoi(master_number_string);
	master_total_size = atoi(master_total_size_string);
	_mount_point=std::string(master_mount_point);
	try
	{
		_init_server();
		_buffer_all_meta_data_from_remote(master_mount_point);
	}
	catch(std::runtime_error)
	{
		throw; 
	}
}

int Master::_setup_queues()
{
	CBB_heart_beat::set_queues(Server::get_communication_input_queue(HEART_BEAT_QUEUE_NUM),
			Server::get_communication_output_queue(HEART_BEAT_QUEUE_NUM));
	return SUCCESS;
}

Master::~Master()
{
	Server::stop_server(); 
	for(IOnode_t::iterator it=_registed_IOnodes.begin();
			it!=_registed_IOnodes.end();++it)
	{
		
		Send(it->second->socket, I_AM_SHUT_DOWN);
		close(it->second->socket);
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

int Master::start_server()
{
	Server::start_server();
	while(true)
	{
		heart_beat_check();
		sleep(HEART_BEAT_INTERVAL);
	}
	return SUCCESS;
}

int Master::remote_task_handler(remote_task* new_task)
{
	int request=new_task->get_task_id();
	switch(request)
	{
		case RENAME:
			_remote_rename(new_task);break;
		case RM_DIR:
			_remote_rmdir(new_task);break;
		case UNLINK:
			_remote_unlink(new_task);break;
		case MKDIR:
			_remote_mkdir(new_task);break;
	}
	return SUCCESS;
}

int Master::_parse_request(extended_IO_task* new_task)
{
	int request=0, ans=SUCCESS; 
	new_task->pop(request); 
	_DEBUG("new request %d\n", request);
	switch(request)
	{
		case REGIST:
			_parse_regist_IOnode(new_task);break;
		case NEW_CLIENT:
			_parse_new_client(new_task);break;
		case OPEN_FILE:
			_parse_open_file(new_task);break; 
		case READ_FILE:
			_parse_read_file(new_task);break;
		case WRITE_FILE:
			_parse_write_file(new_task);break;
		case FLUSH_FILE:
			_parse_flush_file(new_task);break;
		case CLOSE_FILE:
			_parse_close_file(new_task);break;
		case GET_ATTR:
			_parse_attr(new_task);break;
		case READ_DIR:
			_parse_readdir(new_task);break;
		case RM_DIR:
			_parse_rmdir(new_task);break;
		case UNLINK:
			_parse_unlink(new_task);break;
		case ACCESS:
			_parse_access(new_task);break;
		case RENAME:
			_parse_rename(new_task);break;
		case RENAME_MIGRATING:
			_parse_rename_migrating(new_task);break;
		case MKDIR:
			_parse_mkdir(new_task);break;
		case TRUNCATE:
			_parse_truncate_file(new_task);break;
		case CLOSE_CLIENT:
			_parse_close_client(new_task);break;
		case NODE_FAILURE:
			_parse_node_failure(new_task);break;
	}
	_DEBUG("parsing ends\n");
	return ans; 
}

//R: total memory: size_t
//S: node_id: ssize_t
int Master::_parse_regist_IOnode(extended_IO_task* new_task)
{
	struct sockaddr_in addr;
	socklen_t len=sizeof(addr);
	size_t total_memory=0;
	getpeername(new_task->get_socket(), reinterpret_cast<sockaddr*>(&addr), &len); 
	std::string ip=std::string(inet_ntoa(addr.sin_addr));
	_LOG("regist IOnode\nip=%s\n",ip.c_str());
	new_task->pop(total_memory);

	extended_IO_task* output=init_response_task(new_task);

	output->push_back(_add_IOnode(ip, total_memory, output->get_socket()));

	output_task_enqueue(output);
	_DEBUG("total memory %lu\n", total_memory);
	return SUCCESS;
}

//S: SUCCESS: int
int Master::_parse_new_client(extended_IO_task* new_task)
{
	struct sockaddr_in addr;
	socklen_t len=sizeof(addr);
	getpeername(new_task->get_socket(), reinterpret_cast<sockaddr*>(&addr), &len); 
	std::string ip=std::string(inet_ntoa(addr.sin_addr));
	_LOG("new client ip=%s\n", ip.c_str());
	Server::_add_socket(new_task->get_socket()); 

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
int Master::_parse_open_file(extended_IO_task* new_task)
{
	char *file_path=nullptr;
	_LOG("request for open file\n");
	new_task->pop_string(&file_path); 
	int flag=0,ret=SUCCESS;
	ssize_t file_no=0;
	int exist_flag=EXISTING;
	extended_IO_task* output=nullptr;
	std::string str_file_path=std::string(file_path);
	file_stat_pool_t::iterator it=_file_stat_pool.find(str_file_path);

	new_task->pop(flag); 

	if(flag & O_CREAT)
	{
		mode_t mode;
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
		_open_file(file_path, flag, file_no, exist_flag); 
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
int Master::_parse_read_file(extended_IO_task* new_task)
{
	ssize_t file_no=0;
	size_t size=0;
	int ret=0;
	off64_t start_point=0;
	open_file_info *file=nullptr; 
	_LOG("request for reading\n");
	new_task->pop(file_no); 
	extended_IO_task* output=init_response_task(new_task);
	try
	{
		file=_buffered_files.at(file_no);
		new_task->pop(start_point);
		new_task->pop(size);

		block_list_t block_list;
		node_id_pool_t node_pool;
		//_get_blocks_for_IO(start_point, size, *file, IO_blocks);
		_select_node_block_set(*file, start_point, size, node_pool, block_list);
		output=init_response_task(new_task);
		_send_block_info(output, node_pool, block_list);
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
int Master::_parse_write_file(extended_IO_task* new_task)
{
	_LOG("request for writing\n");
	ssize_t file_no=0;
	size_t size=0;
	int ret=0;
	off64_t start_point=0;
	new_task->pop(file_no);
	_DEBUG("file no=%ld\n", file_no);
	extended_IO_task* output=nullptr;
	try
	{
		open_file_info &file=*_buffered_files.at(file_no);
		std::string real_path=_get_real_path(file.get_path());
		Recv_attr(new_task, &file.get_stat());
		new_task->pop(start_point);
		new_task->pop(size);
		_DEBUG("real_path=%s, file_size=%ld\n", real_path.c_str(), file.get_stat().st_size);

		block_list_t block_list;
		node_id_pool_t node_pool;

		_allocate_new_blocks_for_writing(file, start_point, size);
		_select_node_block_set(file, start_point, size, node_pool, block_list);
		output=init_response_task(new_task);
		_send_block_info(output, node_pool, block_list);
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

int Master::_select_node_block_set(open_file_info& file,
		off64_t start_point,
		size_t size,
		node_id_pool_t& node_id_pool,
		block_list_t& block_list)const
{
	//only return the main IOnode
	node_id_pool.insert(file.main_node_id);
	off64_t block_start_point=get_block_start_point(start_point);
	auto block=file.block_list.find(block_start_point);
	for(ssize_t remaining_size=size;0 < remaining_size;remaining_size -= BLOCK_SIZE)
	{
		block_list.insert(make_pair(block->first, block->second));
		//node_id_pool.insert(block->second);
		block_start_point += BLOCK_SIZE;
		block++;
	}
	return SUCCESS;
}

//R: file no: ssize_t
//S: SUCCESS			errno: int
int Master::_parse_flush_file(extended_IO_task* new_task)
{
	_LOG("request for writing\n");
	ssize_t file_no=0;
	int ret=0;
	new_task->pop(file_no);
	extended_IO_task* output=init_response_task(new_task);
	try
	{
		open_file_info &file=*_buffered_files.at(file_no);
		output->push_back(SUCCESS);
		output_task_enqueue(output);
		_DEBUG("write back request to IOnode %ld, file_no %ld\n", file.main_node_id, file_no);
		extended_IO_task* IOnode_output=allocate_output_task(0);
		IOnode_output->set_socket(_registed_IOnodes.at(file.main_node_id)->socket);
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
int Master::_parse_close_file(extended_IO_task* new_task)
{
	_LOG("request for closing file\n");
	ssize_t file_no;
	new_task->pop(file_no);
	extended_IO_task* output=init_response_task(new_task);
	open_file_info* file=nullptr;
	try
	{
		_LOG("file no %ld\n", file_no);
		file=_buffered_files.at(file_no);
		output->push_back(SUCCESS);
		output_task_enqueue(output);
		/*if(NOT_EXIST == file.file_status->exist_flag)
		{
			if(-1 == creat(file.file_status->get_path().c_str(), 0600))
			{
				perror("create");
			}
			file.file_status->exist_flag=EXISTING;
		}*/
	}
	catch(std::out_of_range &e)
	{
		output->push_back(FAILURE);
		output_task_enqueue(output);
		return FAILURE;
	}
	_DEBUG("write back request to IOnode %ld\n", file->main_node_id);
	extended_IO_task* IOnode_output=allocate_output_task(0);
	IOnode_output->set_socket(_registed_IOnodes.at(file->main_node_id)->socket);
	IOnode_output->push_back(CLOSE_FILE);
	IOnode_output->push_back(file_no);
	output_task_enqueue(IOnode_output);
	return SUCCESS;
}

//R: file_path: char[]
//S: SUCCESS			errno: int
//S: attr: struct stat
int Master::_parse_attr(extended_IO_task* new_task)
{
	_DEBUG("requery for File info\n");
	std::string real_path, relative_path;
	int ret;
	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	extended_IO_task* output=init_response_task(new_task);
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
int Master::_parse_readdir(extended_IO_task* new_task)
{
	_DEBUG("requery for dir info\n");
	std::string relative_path;
	std::string real_path;
	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	_DEBUG("file path=%s\n", real_path.c_str());
	dir_t files=_get_file_stat_from_dir(relative_path);
	
	extended_IO_task* output=init_response_task(new_task);
	output->push_back(SUCCESS);
	output->push_back(static_cast<int>(files.size()));
	for(dir_t::const_iterator it=files.begin();
			it!=files.end();++it)
	{
		output->push_back_string(it->c_str(), it->length());
	}

	output_task_enqueue(output);
	return SUCCESS;
}

//R: file path: char[]
//S: SUCCESS				errno: int
int Master::_parse_unlink(extended_IO_task* new_task)
{
	_LOG("request for unlink\n"); 
	int ret;
	std::string relative_path;
	std::string real_path;
	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	extended_IO_task* output=nullptr;
	_LOG("path=%s\n", real_path.c_str());
	file_stat_pool_t::iterator it=_file_stat_pool.find(relative_path);
	if(_file_stat_pool.end() != it)
	{
		Master_file_stat& file_stat=it->second;
		ssize_t fd=file_stat.get_fd();
		_remove_file(fd);
		output=init_response_task(new_task);
		_file_stat_pool.erase(it);
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
	std::string file_path;
	Server::_recv_real_path(new_task, file_path);
	int ret;
	_LOG("path=%s\n", file_path.c_str());
	extended_IO_task* output=init_response_task(new_task);
	if(-1 != rmdir(file_path.c_str()))
	{
		output->push_back(SUCCESS);
		ret=SUCCESS;
	}
	else
	{
		output->push_back(-errno);
		ret=FAILURE;
	}
	output_task_enqueue(output);
	return ret;
}

//R: file path: char[]
//R: mode: int
//S: SUCCESS				errno: int
int Master::_parse_access(extended_IO_task* new_task)
{
	int mode, ret;
	_LOG("request for access\n");
	std::string real_path, relative_path;
	extended_IO_task* output=init_response_task(new_task);
	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	new_task->pop(mode);
	_LOG("path=%s\n, mode=%d", real_path.c_str(), mode);
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
int Master::_parse_mkdir(extended_IO_task* new_task)
{
	_LOG("request for mkdir\n");
	mode_t mode=0;
	extended_IO_task* output=init_response_task(new_task);
	std::string real_path;
	_recv_real_path(new_task, real_path);
	new_task->pop(mode);
	_LOG("path=%s\n", real_path.c_str());
	//make internal dir
	if(-1 == mkdir(real_path.c_str(), mode))
	{
		output->push_back(-errno);
	}
	else
	{
		output->push_back(SUCCESS);
	}
	output_task_enqueue(output);
	return SUCCESS;
}

//R: file path: char[]
//R: file path: char[]
//S: SUCCESS				errno: int
int Master::_parse_rename(extended_IO_task* new_task)
{
	int new_master=0;
	_LOG("request for rename file\n");
	std::string old_real_path, old_relative_path, new_real_path, new_relative_path;
	_recv_real_relative_path(new_task, old_real_path, old_relative_path);
	_recv_real_relative_path(new_task, new_real_path, new_relative_path); 
	_LOG("old file path=%s, new file path=%s\n", old_real_path.c_str(), new_real_path.c_str());
	//unpaired
	new_task->pop(new_master);
	file_stat_pool_t::iterator it=_file_stat_pool.find(old_relative_path);
	if( MYSELF == new_master)
	{
		_file_stat_pool.erase(new_relative_path);
		if((it != _file_stat_pool.end()))
		{
			Master_file_stat& file_stat=it->second;
			file_stat_pool_t::iterator new_it=_file_stat_pool.insert(std::make_pair(new_relative_path, file_stat)).first;
			_file_stat_pool.erase(it);
			new_it->second.full_path=new_it;
			if(nullptr != new_it->second.opened_file_info)
			{
				open_file_info* opened_file = new_it->second.opened_file_info;
				opened_file->file_status=&(new_it->second);
				for(auto node_id:opened_file->IOnodes_set)
				{
					extended_IO_task* output=allocate_output_task(0);
					output->set_socket(_registed_IOnodes.at(node_id)->socket);
					output->push_back(RENAME);
					output->push_back(opened_file->file_no);
					output->push_back_string(new_relative_path.c_str(), new_relative_path.size());
					output_task_enqueue(output);
				}
			}
		}
	}
	else
	{
		if( _file_stat_pool.end() != it)
		{
			it->second.external_flag = RENAMED;
			it->second.external_name = new_relative_path;
		}
	}
	extended_IO_task* output=init_response_task(new_task);
	output->push_back(SUCCESS);
	output_task_enqueue(output);

	return SUCCESS;
}

int Master::_parse_close_client(extended_IO_task* new_task)
{
	_LOG("close client\n");
	int socket=new_task->get_socket();
	Server::_delete_socket(socket);
	close(socket);
	return SUCCESS;
}

//R: file path: char[]
//R: file size: off64_t
//S: SUCCESS				errno: int
int Master::_parse_truncate_file(extended_IO_task* new_task)
{
	_LOG("request for truncate file\n");
	int ret;
	off64_t size;
	extended_IO_task* output=nullptr;
	std::string real_path, relative_path;
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
				for(auto& node_id:file->IOnodes_set)
				{
					output=allocate_output_task(0);
					output->set_socket(_registed_IOnodes.at(node_id)->socket);
					output->push_back(TRUNCATE);
					output->push_back(fd);
					output->push_back(block_start_point);
					output->push_back(size-block_start_point);
					output_task_enqueue(output);
				}
				block_start_point += BLOCK_SIZE;
				while(block_start_point + BLOCK_SIZE <= size)
				{
					_DEBUG("remove block %ld\n", block_start_point);
					file->block_list.erase(block_start_point);
					block_start_point+=BLOCK_SIZE;
				}
			}
			file->get_stat().st_size=size;
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

int Master::_parse_rename_migrating(extended_IO_task* new_task)
{
	_LOG("request for truncate file\n");
	int old_master;
	std::string real_path, relative_path;
	_recv_real_relative_path(new_task, real_path, relative_path);
	new_task->pop(old_master);
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

int Master::_parse_node_failure(extended_IO_task* new_task)
{
	_LOG("parsing node failure\n");
	int socket=new_task->get_socket();
	IOnode_sock_t::const_iterator it=_IOnode_socket.find(socket);
	if(end(_IOnode_socket) != it)
	{
		_recreate_replicas(it->second);
		return _unregist_IOnode(it->second);
	}
	else
	{
		return _close_client(socket);
	}
}

int Master::_recreate_replicas(node_info* IOnode_info)
{
	_LOG("recreating new replicas\n");
	ssize_t node_id=IOnode_info->node_id;
	if(1 == _registed_IOnodes.size())
	{
		//only one node remains
		_remove_IOnode_buffered_file(IOnode_info);
		return SUCCESS;
	}
	file_no_pool_t& stored_files=IOnode_info->stored_files;
	ssize_t replace_IOnode_id=_allocate_new_IOnode();
	for(auto &file_no:stored_files)
	{
		open_file_info* file_info=_buffered_files.at(file_no);	
		//_send_open_request_to_IOnodes(*file_info, replace_IOnode_id, file_info->block_list, file_info->node_id_pool);
		//remove IOnode no from node list in that file
		file_info->IOnodes_set.erase(node_id);
		if(node_id == file_info->main_node_id)
		{
			file_info->main_node_id=*begin(file_info->IOnodes_set);

		}
		file_info->IOnodes_set.insert(replace_IOnode_id);
	}
	return SUCCESS;
}

ssize_t Master::_allocate_new_IOnode()
{
	return (_current_IOnode++)->first;
}

int Master::_unregist_IOnode(node_info* IOnode_info)
{
	ssize_t node_id=IOnode_info->node_id;
	int socket=IOnode_info->socket;
	_LOG("unregist IOnode id %ld\n", node_id);
	_IOnode_socket.erase(socket);
	_registed_IOnodes.erase(node_id);
	delete IOnode_info;
	return SUCCESS;
}

int Master::_remove_IOnode_buffered_file(node_info* IOnode_info)
{
	file_no_pool_t& stored_files=IOnode_info->stored_files;
	ssize_t const node_id=IOnode_info->node_id;
	for(auto &file_no:stored_files)
	{
		open_file_info* file_info=_buffered_files.at(file_no);	
		//remove IOnode no from node list in that file
		file_info->IOnodes_set.erase(node_id);
	}
	return SUCCESS;
}

int Master::_close_client(int socket)
{
	_LOG("closing client socket %d\n", socket);
	Server::_delete_socket(socket);
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
void Master::_send_block_info(Common::extended_IO_task* output,
		const node_id_pool_t& node_id_pool,
		const block_list_t& block_list)const
{
	output->push_back(SUCCESS);
	output->push_back(static_cast<int>(node_id_pool.size()));
	for(auto& node_id:node_id_pool)
	{
		output->push_back(node_id);
		const std::string& ip=_registed_IOnodes.at(node_id)->ip;
		output->push_back_string(ip.c_str(), ip.size());
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
		output->set_socket(_registed_IOnodes.at(nodes_it->first)->socket);
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
int Master::_send_open_request_to_IOnodes(struct open_file_info& file,
		ssize_t current_node_id,
		const block_list_t& block_info,
		const node_id_pool_t& IOnodes_set)
{
	//send read request to each IOnode
	//buffer requset, file_no, open_flag, exist_flag, file_path, start_point, block_size
	ssize_t main_node_id=file.main_node_id;
	int socket=_registed_IOnodes.at(current_node_id)->socket;
	extended_IO_task* output=allocate_output_task(0);
	output->set_socket(socket);
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
	if(current_node_id == file.main_node_id)
	{
		output->push_back(MAIN_REPLICA);
		output->push_back(static_cast<int>(IOnodes_set.size())-1);
		for(auto& node_id:IOnodes_set)
		{
			if(node_id != current_node_id)
			{
				node_info* node=_registed_IOnodes.at(node_id);
				output->push_back(node_id);
				output->push_back_string(node->ip.c_str());
			}
		}
	}
	else
	{
		output->push_back(SUB_REPLICA);
		output->push_back(static_cast<int>(0));
	}
	output_task_enqueue(output);
	return SUCCESS; 
}

ssize_t Master::_add_IOnode(const std::string& node_ip,
		std::size_t total_memory,
		int socket)
{
	ssize_t id=0; 
	if(-1 == (id=_get_node_id()))
	{
		return -1;
	}
	if(_registed_IOnodes.end() != _find_by_ip(node_ip))
	{
		return -1;
	}
	_registed_IOnodes.insert(std::make_pair(id, new node_info(id, node_ip, total_memory, socket)));
	node_info *node=_registed_IOnodes.at(id); 
	_IOnode_socket.insert(std::make_pair(socket, node)); 
	Server::_add_socket(socket); 
	return id; 
}

ssize_t Master::_delete_IOnode(int socket)
{
	node_info *node=_IOnode_socket.at(socket);
	ssize_t id=node->node_id;
	_registed_IOnodes.erase(id);
	Server::_delete_socket(socket); 
	close(socket); 
	_IOnode_socket.erase(socket); 
	_node_id_pool[id]=false;
	delete node;
	return id;
}

const node_id_pool_t& Master::_open_file(const char* file_path,
		int flag,
		ssize_t& file_no,
		int exist_flag)throw(std::runtime_error, std::invalid_argument, std::bad_alloc)
{
	file_stat_pool_t::iterator it; 
	open_file_info *file=nullptr; 
	//file not buffered
	if(_file_stat_pool.end() ==  (it=_file_stat_pool.find(file_path)))
	{
		file_no=_get_file_no(); 
		if(-1 != file_no)
		{
			Master_file_stat* file_status=_create_new_file_stat(file_path, exist_flag);
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
	if(MAX_NODE_NUMBER == _registed_IOnodes.size())
	{
		return -1;
	}
	for(; _current_node_number<MAX_NODE_NUMBER; ++_current_node_number)
	{
		if(!_node_id_pool[_current_node_number])
		{
			_node_id_pool[_current_node_number]=true; 
			return _current_node_number; 
		}
	}
	for(_current_node_number=0;  _current_node_number<MAX_NODE_NUMBER; ++_current_node_number)
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

void Master::_release_file_no(ssize_t file_no)
{
	--_file_number;
	_file_no_pool[file_no]=false;
	return;
}

Master::IOnode_t::iterator Master::_find_by_ip(const std::string &ip)
{
	IOnode_t::iterator it=_registed_IOnodes.begin();
	for(; it != _registed_IOnodes.end() && ! (it->second->ip == ip); ++it);
	return it;
}

open_file_info* Master::_create_new_open_file_info(ssize_t file_no,
		int flag,
		Master_file_stat* stat)throw(std::invalid_argument)
{
	try
	{
		open_file_info *new_file=new open_file_info(file_no, flag, stat); 
		_buffered_files.insert(std::make_pair(file_no, new_file));
		new_file->block_size=get_block_size(stat->get_status().st_size); 
		stat->opened_file_info=new_file;
		new_file->main_node_id=_select_IOnode(file_no, MIN(NUM_OF_REPLICA, _registed_IOnodes.size()), new_file->IOnodes_set);
		_create_block_list(new_file->get_stat().st_size, new_file->block_size, new_file->block_list, new_file->IOnodes_set);
		for(auto& IOnode_id:new_file->IOnodes_set)
		{
			_send_open_request_to_IOnodes(*new_file, IOnode_id, new_file->block_list, new_file->IOnodes_set);
		}
		return new_file;
	}
	catch(std::invalid_argument &e)
	{
		//file open error
		_release_file_no(file_no);
		throw;
	}
}

Master_file_stat* Master::_create_new_file_stat(const char* relative_path,
		int exist_flag)throw(std::invalid_argument)
{
	std::string relative_path_string = std::string(relative_path);
	std::string real_path=_get_real_path(relative_path_string);
	_DEBUG("file path=%s\n", real_path.c_str());
	struct stat file_status;

	time_t current_time;
	memset(&file_status, 0, sizeof(file_status));
	time(&current_time);
	file_status.st_mtime=current_time;
	file_status.st_atime=current_time;
	file_status.st_ctime=current_time;
	file_status.st_size=0;
	file_status.st_uid=getuid();
	file_status.st_gid=getgid();
	file_status.st_mode=S_IFREG|0664;
	_DEBUG("add new file states\n");
	file_stat_pool_t::iterator it=_file_stat_pool.insert(std::make_pair(relative_path_string, Master_file_stat())).first;
	Master_file_stat& new_file_stat=it->second;
	new_file_stat.full_path=it;
	new_file_stat.exist_flag=exist_flag;
	memcpy(&new_file_stat.get_status(), &file_status, sizeof(file_status));
	return &new_file_stat;
}

ssize_t Master::_select_IOnode(ssize_t file_no,
		int num_of_nodes,
		node_id_pool_t& node_id_pool)
{
	if(0 == _registed_IOnodes.size() || 0 == num_of_nodes)
	{
		//no IOnode
		return -1;
	}
	if(end(_registed_IOnodes) == _current_IOnode)
	{
		_current_IOnode=begin(_registed_IOnodes);
	}
	//insert main replica
	ssize_t main_replica=_current_IOnode->first;
	_current_IOnode->second->stored_files.insert(file_no);
	node_id_pool.insert((_current_IOnode++)->first);
	//insert sub replica
	auto _replica_IOnode=_current_IOnode;
	for(;0 < num_of_nodes;num_of_nodes--)
	{
		if(end(_registed_IOnodes) == _replica_IOnode)
		{
			_replica_IOnode=begin(_registed_IOnodes);
		}
		_replica_IOnode->second->stored_files.insert(file_no);
		node_id_pool.insert((_replica_IOnode++)->first);
	}
	return main_replica; 
}

int Master::_create_block_list(size_t file_size,
		size_t block_size,
		block_list_t& block_list,
		node_id_pool_t& node_list)
{
	off64_t block_start_point=0;
	size_t remaining_size=file_size;
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

int Master::_allocate_new_blocks_for_writing(open_file_info& file,
		off64_t start_point,
		size_t size)
{
	off64_t current_point=get_block_start_point(start_point, size);
	block_list_t::iterator current_block=file.block_list.find(current_point); ssize_t remaining_size=size;
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
		size_t block_size=MIN(remaining_size, BLOCK_SIZE);
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
		node_id_pool_t& node_id_pool)throw(std::bad_alloc)
{
	//temp solution
	int node_id=*file.nodes.begin();
	for(auto nodes:file.nodes)
	{
		node_id_pool.insert(nodes);
	}
	//node_info &selected_node=_registed_IOnodes.at(node_id);
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

void Master::_append_block(struct open_file_info& file, off64_t start_point, size_t size)
{
	_DEBUG("append block=%ld\n", start_point);
	file.block_list.insert(std::make_pair(start_point, size));
	//file.IOnodes_set.insert(node_id);
}

int Master::_remove_file(ssize_t file_no)
{
	File_t::iterator it=_buffered_files.find(file_no);
	if(_buffered_files.end() != it)
	{
		open_file_info &file=*(it->second);
		for(auto IOnode_id:file.IOnodes_set)
		{
			extended_IO_task* output=allocate_output_task(0);
			node_info* IOnode=_registed_IOnodes.at(IOnode_id);
			output->set_socket(IOnode->socket);
			output->push_back(UNLINK);
			_DEBUG("unlink file no=%ld\n", file_no);
			output->push_back(file_no);
			output_task_enqueue(output);
			//remove file_no from stored files list
			IOnode->stored_files.erase(file_no);
		}
		_buffered_files.erase(file_no);
		delete &file;
		_release_file_no(file_no);
	}
	return SUCCESS;
}

//low performance
Master::dir_t Master::_get_file_stat_from_dir(const std::string& dir)
{
	dir_t files;
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
	return files;
}

int Master::_buffer_all_meta_data_from_remote(const char* mount_point)throw(std::runtime_error)
{
	char file_path[PATH_MAX];
	DIR *dir=opendir(mount_point);
	if(nullptr == dir)
	{
		_DEBUG("file path%s\n", mount_point);
		perror("open remote storage");
		throw std::runtime_error("open remote storage");
	}
	strcpy(file_path, mount_point);

	_LOG("start to buffer file status from remote\n");
	struct stat file_status;
	lstat(file_path, &file_status);
	file_stat_pool_t::iterator it=_file_stat_pool.insert(std::make_pair("/", Master_file_stat(file_status, "/", EXISTING))).first;
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

int Master::_dfs_items_in_remote(DIR* current_remote_directory,
		char* file_path,
		const char* file_relative_path,
		size_t offset)throw(std::runtime_error)
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
			file_stat_pool_t::iterator it=_file_stat_pool.insert(std::make_pair(file_relative_path, Master_file_stat(file_status, dir_item->d_name, EXISTING))).first;
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

int Master::get_IOnode_socket_map(socket_map_t& socket_map)
{
	for(const auto& IOnode:_IOnode_socket)
	{
		socket_map.insert(std::make_pair(IOnode.first, IOnode.second->node_id));
	}
	return SUCCESS;
}

int Master::node_failure_handler(int node_socket)
{
	//dummy code
	_DEBUG("IOnode failed id=%d\n", node_socket);
	return send_input_for_socket_error(node_socket);
}

int Master::_remote_rename(Common::remote_task* new_task)
{
	string* old_name=static_cast<string*>(new_task->get_task_data()), *new_name=static_cast<string*>(new_task->get_extended_task_data());
	std::string old_real_path=_get_real_path(*old_name), new_real_path=_get_real_path(*new_name);
	int ret=SUCCESS;

	_LOG("rename %s to %s\n", old_real_path.c_str(), new_real_path.c_str());
	if(0 != (ret=rename(old_real_path.c_str(), new_real_path.c_str())))
	{
		perror("rename");
	}
	delete old_name;
	return ret;
}

int Master::_remote_rmdir(Common::remote_task* new_task)
{
	int ret=SUCCESS;
	string* dir_name=static_cast<string*>(new_task->get_task_data());
	std::string real_dir_name=_get_real_path(*dir_name);

	_LOG("rmdir %s\n", real_dir_name.c_str());
	if(0 != (ret=rmdir(real_dir_name.c_str())))
	{
		perror("rmdir");
	}

	return ret;
}

int Master::_remote_unlink(Common::remote_task* new_task)
{
	int ret=SUCCESS;
	string* file_name=static_cast<string*>(new_task->get_task_data());
	std::string real_file_name=_get_real_path(*file_name);

	_LOG("unlink file %s\n", real_file_name.c_str());
	if(0 != (ret=unlink(real_file_name.c_str())))
	{
		perror("unlink");
	}

	return ret;
}

int Master::_remote_mkdir(Common::remote_task* new_task)
{
	int ret=SUCCESS;
	string* dir_name=static_cast<string*>(new_task->get_task_data());
	mode_t* mode=static_cast<mode_t*>(new_task->get_extended_task_data());
	std::string real_dir_name=_get_real_path(*dir_name);

	_LOG("mkdir %s\n", real_dir_name.c_str());
	if(0 != (ret=mkdir(real_dir_name.c_str(), *mode)))
	{
		perror("mkdir");
	}

	return ret;
}
