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
const char *Master::MASTER_MOUNT_POINT="CBB_MASTER_MOUNT_POINT";
const char *Master::MASTER_NUMBER="CBB_MASTER_MY_ID";
const char *Master::MASTER_TOTAL_NUMBER="CBB_MASTER_TOTAL_NUMBER";

Master::Master()throw(std::runtime_error):
	Server(MASTER_PORT), 
	_registed_IOnodes(IOnode_t()), 
	_file_stat_pool(),
	_buffered_files(), 
	_IOnode_socket(IOnode_sock_t()), 
	_node_number(0), 
	_file_number(0), 
	_node_id_pool(new bool[MAX_NODE_NUMBER]), 
	_file_no_pool(new bool[MAX_FILE_NUMBER]), 
	_current_node_number(0), 
	_current_file_no(0),
	_mount_point(std::string()),
	_current_IOnode(_registed_IOnodes.end())
{
	memset(_node_id_pool, 0, MAX_NODE_NUMBER*sizeof(bool)); 
	memset(_file_no_pool, 0, MAX_FILE_NUMBER*sizeof(bool)); 
	const char *master_mount_point=getenv(MASTER_MOUNT_POINT);
	const char *master_number_string = getenv(MASTER_NUMBER);
	const char *master_total_size_string= getenv(MASTER_TOTAL_NUMBER);
	
	if(NULL == master_mount_point)
	{
		throw std::runtime_error("please set master mount point");
	}
	if(NULL == master_number_string)
	{
		throw std::runtime_error("please set master number");
	}
	if(NULL == master_total_size_string)
	{
		throw std::runtime_error("please set master total number");
	}
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

/*int Master::_parse_new_request(int clientfd,
		const struct sockaddr_in& client_addr)
{
	int request, ans=SUCCESS;
	std::string ip(inet_ntoa(client_addr.sin_addr));
	Recv(clientfd, request);
	switch(request)
	{
	default:
		Send(clientfd, UNRECOGNISTED); 
		close(clientfd); 
	}
	return ans;
}*/

int Master::start_server()
{
	Server::start_server();
	while(1)
	{
		sched_yield();
		sleep(1000000);
	}
	return SUCCESS;
}

int Master::_parse_request(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	int request=0, ans=SUCCESS; 
	new_task->pop(request); 
	_DEBUG("new request %d\n", request);
	switch(request)
	{
		//case PRINT_NODE_INFO:
		//	_send_node_info(clientfd);break;
		case REGIST:
			_parse_regist_IOnode(new_task, output_queue);break;
		case NEW_CLIENT:
			_parse_new_client(new_task, output_queue);break;
		case OPEN_FILE:
			_parse_open_file(new_task, output_queue);break; 
		case READ_FILE:
			_parse_read_file(new_task, output_queue);break;
		case WRITE_FILE:
			_parse_write_file(new_task, output_queue);break;
		case FLUSH_FILE:
			_parse_flush_file(new_task, output_queue);break;
		case CLOSE_FILE:
			_parse_close_file(new_task, output_queue);break;
			//case GET_FILE_META:
			//	_send_file_meta(clientfd);break;
		case GET_ATTR:
			_parse_attr(new_task, output_queue);break;
		case READ_DIR:
			_parse_readdir(new_task, output_queue);break;
		case RM_DIR:
			_parse_rmdir(new_task, output_queue);break;
		case UNLINK:
			_parse_unlink(new_task, output_queue);break;
		case ACCESS:
			_parse_access(new_task, output_queue);break;
		case RENAME:
			_parse_rename(new_task, output_queue);break;
		case RENAME_MIGRATING:
			_parse_rename_migrating(new_task, output_queue);break;
		case MKDIR:
			_parse_mkdir(new_task, output_queue);break;
		case TRUNCATE:
			_parse_truncate_file(new_task, output_queue);break;
		case CLOSE_CLIENT:
			_parse_close_client(new_task, output_queue);break;
			/*case I_AM_SHUT_DOWN:
			  id=_IOnode_socket.at(clientfd)->node_id;
			  _DEBUG("IOnode %ld shutdown\nIP Address=%s, Unregisted\n", id, _registed_IOnodes.at(id)->ip.c_str()); 
			  _delete_IO_node(clientfd);break;*/
			//default:
			//Send_flush(clientfd, UNRECOGNISTED); 
	}
	return ans; 
}

//R: total memory: size_t
//S: node_id: ssize_t
int Master::_parse_regist_IOnode(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	struct sockaddr_in addr;
	socklen_t len=sizeof(addr);
	size_t total_memory=0;
	getpeername(new_task->get_socket(), reinterpret_cast<sockaddr*>(&addr), &len); 
	std::string ip=std::string(inet_ntoa(addr.sin_addr));
	_LOG("regist IOnode\nip=%s\n",ip.c_str());
	new_task->pop(total_memory);

	IO_task* output=CBB::Common::init_response_task(new_task, output_queue);

	output->push_back(_add_IO_node(ip, total_memory, output->get_socket()));

	output_queue->task_enqueue();
	_DEBUG("total memory %lu\n", total_memory);
	return SUCCESS;
}

//S: SUCCESS: int
int Master::_parse_new_client(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	struct sockaddr_in addr;
	socklen_t len=sizeof(addr);
	getpeername(new_task->get_socket(), reinterpret_cast<sockaddr*>(&addr), &len); 
	std::string ip=std::string(inet_ntoa(addr.sin_addr));
	_LOG("new client ip=%s\n", ip.c_str());
	Server::_add_socket(new_task->get_socket()); 

	IO_task* output=CBB::Common::init_response_task(new_task, output_queue);

	output->push_back(SUCCESS);

	output_queue->task_enqueue();
	return SUCCESS;
}

//rename-open issue
//R: file_path: char[]
//R: flag: int
//S: SUCCESS			errno: int
//S: file no: ssize_t
//S: block size: size_t
//S: attr: struct stat
int Master::_parse_open_file(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	char *file_path=NULL;
	_LOG("request for open file\n");
	new_task->pop_string(&file_path); 
	int flag=0,ret=SUCCESS;
	ssize_t file_no=0;
	int exist_flag=EXISTING;
	//bad hack, fix it later
	std::string str_file_path=std::string(file_path);
	file_stat_pool_t::iterator it=_file_stat_pool.find(str_file_path);

	new_task->pop(flag); 

	if(flag & O_CREAT)
	{
		mode_t mode;
		new_task->pop(mode);
		exist_flag=NOT_EXIST;
		//_create_file(file_path, mode);
		flag &= ~(O_CREAT | O_TRUNC);
	}
	if((_file_stat_pool.end() != it))
	{
		Master_file_stat& file_stat=it->second;
		if(file_stat.is_external())
		{
			IO_task* output=init_response_task(new_task, output_queue);
			output->push_back(file_stat.external_master);
			output_queue->task_enqueue();
			return SUCCESS;
		}
	}
	new_task->pop(master_number);
	try
	{
		_open_file(file_path, flag, file_no, exist_flag, output_queue); 
		open_file_info *opened_file=_buffered_files.at(file_no);
		size_t block_size=opened_file->block_size;
		_DEBUG("file path= %s, file no=%ld\n", file_path, file_no);
		IO_task* output=init_response_task(new_task, output_queue);
		output->push_back(SUCCESS);
		output->push_back(file_no);
		output->push_back(block_size);
		Send_attr(output, &opened_file->get_stat());
	}
	catch(std::runtime_error &e)
	{
		_DEBUG("%s\n",e.what());
		IO_task* output=init_response_task(new_task, output_queue);
		output->push_back(UNKNOWN_ERROR);
		ret=FAILURE;
	}
	catch(std::invalid_argument &e)
	{
		_DEBUG("%s\n",e.what());
		IO_task* output=init_response_task(new_task, output_queue);
		output->push_back(FILE_NOT_FOUND);
		ret=FAILURE;
	}
	catch(std::bad_alloc &e)
	{
		IO_task* output=init_response_task(new_task, output_queue);
		output->push_back(TOO_MANY_FILES);
		ret=FAILURE;
	}
	_DEBUG("here\n");
	output_queue->task_enqueue();
	return ret;
}

//R: file no: ssize_t
//S: SUCCESS			errno: int
//R: start point: off64_t
//R: size: size_t
//S: block_info
int Master::_parse_read_file(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	ssize_t file_no=0;
	size_t size=0;
	int ret=0;
	off64_t start_point=0;
	open_file_info *file=NULL; 
	_LOG("request for reading\n");
	new_task->pop(file_no); 
	IO_task* output=init_response_task(new_task, output_queue);
	try
	{
		file=_buffered_files.at(file_no);
		new_task->pop(start_point);
		new_task->pop(size);
		node_t nodes;
		node_id_pool_t node_pool;
		_get_IOnodes_for_IO(start_point, size, *file, nodes, node_pool, output_queue);
		_send_block_info(output, node_pool, nodes);
		ret=SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		output->push_back(OUT_OF_RANGE);
		ret=FAILURE; 
	}
	output_queue->task_enqueue();
	return ret;

}

//R: file no: ssize_t
//S: SUCCESS			errno: int
//R: attr: struct stat
//R: start point: off64_t
//R: size: size_t
//S: send_block
int Master::_parse_write_file(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	_LOG("request for writing\n");
	ssize_t file_no=0;
	size_t size=0;
	int ret=0;
	off64_t start_point=0;
	new_task->pop(file_no);
	_DEBUG("file no=%ld\n", file_no);
	IO_task* output=NULL;
	try
	{
		open_file_info &file=*_buffered_files.at(file_no);
		std::string real_path=_get_real_path(file.get_path());
		Recv_attr(new_task, &file.get_stat());
		new_task->pop(start_point);
		new_task->pop(size);
		_DEBUG("real_path=%s, file_size=%ld\n", real_path.c_str(), file.get_stat().st_size);

		node_t nodes;
		node_id_pool_t node_pool;
		_get_IOnodes_for_IO(start_point, size, file, nodes, node_pool, output_queue);
		output=init_response_task(new_task, output_queue);
		_send_block_info(output, node_pool, nodes);
		ret=SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		output=init_response_task(new_task, output_queue);
		output->push_back(OUT_OF_RANGE);
		ret=FAILURE;
	}
	output_queue->task_enqueue();
	return ret;
}

//R: file no: ssize_t
//S: SUCCESS			errno: int
int Master::_parse_flush_file(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	_LOG("request for writing\n");
	ssize_t file_no=0;
	int ret=0;
	new_task->pop(file_no);
	IO_task* output=init_response_task(new_task, output_queue);
	try
	{
		open_file_info &file=*_buffered_files.at(file_no);
		output->push_back(SUCCESS);
		file.nodes.rd_lock();
		for(node_pool_t::iterator it=file.nodes.begin();
				it != file.nodes.end();++it)
		{
			_DEBUG("write back request to IOnode %ld, file_no %ld\n", *it, file_no);
			IO_task* IOnode_output=output_queue->allocate_tmp_node();
			IOnode_output->set_socket(_IOnode_socket.at(*it)->socket);
			IOnode_output->push_back(FLUSH_FILE);
			IOnode_output->push_back(file_no);
			output_queue->task_enqueue();
		}
		file.nodes.unlock();

		ret=SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		IO_task* output=init_response_task(new_task, output_queue);
		output->push_back(FAILURE);
		ret=FAILURE;
	}
	output_queue->task_enqueue();
	return ret;
}

//R: file no: ssize_t
//S: SUCCESS			errno: int
int Master::_parse_close_file(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	_LOG("request for closing file\n");
	ssize_t file_no;
	int ret;
	new_task->pop(file_no);
	IO_task* output=init_response_task(new_task, output_queue);
	try
	{
		_LOG("file no %ld\n", file_no);
		//open_file_info &file=*_buffered_files.at(file_no);
		output->push_back(SUCCESS);
		/*if(NOT_EXIST == file.file_status->exist_flag)
		{
			if(-1 == creat(file.file_status->get_path().c_str(), 0600))
			{
				perror("create");
			}
			file.file_status->exist_flag=EXISTING;
		}*/
		//if(0 == --file.open_count)
		//{

		/*for(node_pool_t::iterator it=file.nodes.begin();
				it != file.nodes.end();++it)
		{
			_DEBUG("write back request to IOnode %ld\n", *it);
			int socket=_registed_IOnodes.at(*it)->socket;
			//Send(socket, FLUSH_FILE);
			//Send(socket, CLOSE_FILE);
			//Send_flush(socket, file_no);
			int ret;
			//Recv(socket, ret);
			//			int ret=0;
			//			Recv(socket, ret);
		}*/
		//_file_stat_pool::iterator file=_buffered_files.find(file_no);
		//std::string &path=file->second.path;
		//_file_no.erase(path);
		//_buffered_files.erase(file);
		//}
		ret=SUCCESS;

	}
	catch(std::out_of_range &e)
	{
		output->push_back(FAILURE);
		ret=FAILURE;
	}
	output_queue->task_enqueue();
	return ret;
}

//R: file_path: char[]
//S: SUCCESS			errno: int
//S: attr: struct stat
int Master::_parse_attr(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	_DEBUG("requery for File info\n");
	std::string real_path, relative_path;
	int ret;
	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	IO_task* output=init_response_task(new_task, output_queue);
	_DEBUG("file path=%s\n", real_path.c_str());
	try
	{
		_file_stat_pool.rd_lock();
		Master_file_stat& file_stat=_file_stat_pool.at(relative_path);
		_file_stat_pool.unlock();
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
			ret=SUCCESS;
		}
	}
	catch(std::out_of_range &e)
	{
		output->push_back(master_number);
		/*try
		{
			Master_file_stat* new_file=_create_new_file_stat_pool(relative_path.c_str(), EXISTING);
			Send(clientfd, SUCCESS);
			Send_attr(clientfd, &(new_file_stat_poolus->fstat));
			return SUCCESS;
		}*/
		output->push_back(-ENOENT);
		ret=FAILURE;
	}
	output_queue->task_enqueue();
	return ret;
}

//R: file path: char[]
//S: SUCCESS				errno: int
//S: dir_count: int
//for dir_count
	//S: file name: char[]
//S: SUCCESS: int
int Master::_parse_readdir(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	_DEBUG("requery for dir info\n");
	std::string relative_path;
	std::string real_path;
	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	_DEBUG("file path=%s\n", real_path.c_str());
	//DIR *dir=opendir(real_path.c_str());
	//const struct dirent* entry=NULL;
	dir_t files=_get_file_stat_from_dir(relative_path);
	
	IO_task* output=init_response_task(new_task, output_queue);
	//while(NULL != (entry=readdir(dir)))
	//{
	//	files.insert(std::string(entry->d_name));
	//}
	output->push_back(SUCCESS);
	output->push_back(static_cast<int>(files.size()));
	for(dir_t::const_iterator it=files.begin();
			it!=files.end();++it)
	{
		output->push_back_string(it->c_str(), it->length());
	}

	output_queue->task_enqueue();
	//TO-DO:fix client side
	//output->push_back(SUCCESS);
	return SUCCESS;
}

//R: file path: char[]
//S: SUCCESS				errno: int
int Master::_parse_unlink(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	_LOG("request for unlink\n"); 
	int ret;
	std::string relative_path;
	std::string real_path;
	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	IO_task* output=init_response_task(new_task, output_queue);
	_LOG("path=%s\n", real_path.c_str());
	try
	{
		Master_file_stat& file_stat=_file_stat_pool.at(relative_path);
		ssize_t fd=file_stat.get_fd();
		_remove_file(fd, output_queue);
		//unlink(real_path.c_str());
		output->push_back(SUCCESS);

		ret=SUCCESS;
	}
	catch(std::out_of_range& e)
	{
		output->push_back(-ENOENT);
		ret=FAILURE;
		/*if(-1 != unlink(real_path.c_str()))
		{
			Send_flush(clientfd, SUCCESS);
			return SUCCESS;
		}
		else
		{
			Send_flush(clientfd, -errno);
			return FAILURE;
		}*/
	}
	output_queue->task_enqueue();
	return ret;
}

//R: file path: char[]
//S: SUCCESS				errno: int
int Master::_parse_rmdir(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	_LOG("request for rmdir\n");
	std::string file_path;
	Server::_recv_real_path(new_task, file_path);
	int ret;
	_LOG("path=%s\n", file_path.c_str());
	IO_task* output=init_response_task(new_task, output_queue);
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
	output_queue->task_enqueue();
	return ret;
}

//R: file path: char[]
//R: mode: int
//S: SUCCESS				errno: int
int Master::_parse_access(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	int mode, ret;
	_LOG("request for access\n");
	std::string real_path, relative_path;
	IO_task* output=init_response_task(new_task, output_queue);
	Server::_recv_real_relative_path(new_task, real_path, relative_path);
	new_task->pop(mode);
	_LOG("path=%s\n, mode=%d", real_path.c_str(), mode);
	try
	{
		_file_stat_pool.rd_lock();
		const Master_file_stat& file_stat=_file_stat_pool.at(relative_path);
		_file_stat_pool.unlock();
		
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
		/*if(-1 != access(real_path.c_str(), mode))
		{
			_LOG("SUCCESS\n");
			Send_flush(clientfd, SUCCESS);
			return SUCCESS;
		}
		else
		{
			Send_flush(clientfd, errno);
			return FAILURE;
		}*/
	}
	output_queue->task_enqueue();
	return ret;
}

//R: file path: char[]
//R: mode: mode_t
//S: SUCCESS				errno: int
int Master::_parse_mkdir(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	_LOG("request for mkdir\n");
	mode_t mode=0;
	IO_task* output=init_response_task(new_task, output_queue);
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
	output_queue->task_enqueue();
	return SUCCESS;
}

//R: file path: char[]
//R: file path: char[]
//S: SUCCESS				errno: int
int Master::_parse_rename(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
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
			if(NULL != new_it->second.opened_file_info)
			{
				open_file_info* opened_file = new_it->second.opened_file_info;
				opened_file->file_status=&(new_it->second);
				for(node_pool_t::iterator it=opened_file->nodes.begin();
						it!=opened_file->nodes.end();++it)
				{
					IO_task* output=output_queue->allocate_tmp_node();
					output->set_socket(_registed_IOnodes.at(*it)->socket);
					output->push_back(RENAME);
					output->push_back(opened_file->file_no);
					output->push_back_string(new_relative_path.c_str(), new_relative_path.size());
				}
			}
		}
		//rename(old_real_path.c_str(), new_real_path.c_str());
	}
	else
	{
		if( _file_stat_pool.end() != it)
		{
			it->second.external_flag = RENAMED;
			it->second.external_name = new_relative_path;
		}
	}
	IO_task* output=init_response_task(new_task, output_queue);
	output->push_back(SUCCESS);
	output_queue->task_enqueue();

	return SUCCESS;
}

int Master::_parse_close_client(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
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
int Master::_parse_truncate_file(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	_LOG("request for truncate file\n");
	int ret;
	off64_t size;
	std::string real_path, relative_path;
	_recv_real_relative_path(new_task, real_path, relative_path);
	new_task->pop(size);
	_LOG("path=%s\n", real_path.c_str());
	try
	{
		Master_file_stat& file_stat=_file_stat_pool.at(relative_path);
		if(file_stat.is_external())
		{
			IO_task* output=init_response_task(new_task, output_queue);
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
				for(off64_t block_start_point=0;
						file->get_stat().st_size > static_cast<off64_t>(block_start_point+BLOCK_SIZE);
						block_start_point+=BLOCK_SIZE)
				{
					node_t::iterator it=file->p_node.find(block_start_point);
					ssize_t node_id=it->second;
					IO_task* output=output_queue->allocate_tmp_node();
					output->set_socket(_registed_IOnodes.at(node_id)->socket);
					output->push_back(TRUNCATE);
					output->push_back(fd);
					output->push_back(block_start_point);
					output_queue->task_enqueue();
					file->p_node.erase(it);
				}
			}
			file->get_stat().st_size=size;
			IO_task* output=init_response_task(new_task, output_queue);
			output->push_back(master_number);
			output->push_back(SUCCESS);
			ret=SUCCESS;
		}
	}
	catch(std::out_of_range &e)
	{
		IO_task* output=init_response_task(new_task, output_queue);
		output->push_back(master_number);
		output->push_back(-ENOENT);
		ret=FAILURE;
	}
	output_queue->task_enqueue();
	//int ret=truncate(real_path.c_str(), size); 
	return ret;
}

int Master::_parse_rename_migrating(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	_LOG("request for truncate file\n");
	int old_master;
	std::string real_path, relative_path;
	_recv_real_relative_path(new_task, real_path, relative_path);
	new_task->pop(old_master);
	_LOG("path=%s\n", relative_path.c_str());
	IO_task* output=init_response_task(new_task, output_queue);
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
	output_queue->task_enqueue();
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
void Master::_send_block_info(IO_task* output,
		const node_id_pool_t& node_pool,
		const node_t& node_set)const
{
	output->push_back(SUCCESS);
	output->push_back(static_cast<int>(node_pool.size()));
	for(node_id_pool_t::const_iterator it=node_pool.begin();
			it!=node_pool.end();++it)
	{
		output->push_back(*it);
		const std::string& ip=_registed_IOnodes.at(*it)->ip;
		output->push_back_string(ip.c_str(), ip.size());
	}

	output->push_back(static_cast<int>(node_set.size()));

	for(node_t::const_iterator it=node_set.begin(); it!=node_set.end(); ++it)
	{
		output->push_back(it->first);
		output->push_back(it->second);
	}
	//Send_flush(clientfd, SUCCESS);
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
		const node_block_map_t& append_blocks,
		task_parallel_queue<IO_task>* output_queue)
{
	for(node_block_map_t::const_iterator nodes_it=append_blocks.begin();
			nodes_it != append_blocks.end();++nodes_it)
	{
		IO_task* output=output_queue->allocate_tmp_node();
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
		output_queue->task_enqueue();
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
int Master::_send_open_request_to_IOnodes(struct open_file_info& file, task_parallel_queue<IO_task>* output_queue)
{
	node_block_map_t node_block_map;
	file.p_node=_select_IOnode(0, file.get_stat().st_size, file.block_size, node_block_map);
	file._set_nodes_pool();
	for(node_block_map_t::const_iterator it=node_block_map.begin(); 
			node_block_map.end()!=it; ++it)
	{
		//send read request to each IOnode
		//buffer requset, file_no, open_flag, exist_flag, file_path, start_point, block_size
		IO_task* output=output_queue->allocate_tmp_node();
		output->set_socket(_registed_IOnodes.at(it->first)->socket);
		output->push_back(OPEN_FILE);
		output->push_back(file.file_no); 
		output->push_back(file.flag);
		output->push_back(file.file_status->exist_flag);
		output->push_back_string(file.get_path().c_str(), file.get_path().length());
		_DEBUG("%s\n", file.get_path().c_str());
		
		output->push_back(static_cast<int>(it->second.size()));
		for(block_info_t::const_iterator blocks_it=it->second.begin();
			blocks_it!=it->second.end(); ++blocks_it)
		{
			output->push_back(blocks_it->first);
			output->push_back(blocks_it->second);
		}
		output_queue->task_enqueue();
	}
	return SUCCESS; 
}

ssize_t Master::_add_IO_node(const std::string& node_ip,
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
	++_node_number; 
	return id; 
}

ssize_t Master::_delete_IO_node(int socket)
{
	node_info *node=_IOnode_socket.at(socket);
	ssize_t id=node->node_id;
	_registed_IOnodes.erase(id);
	Server::_delete_socket(socket); 
	close(socket); 
	_IOnode_socket.erase(socket); 
	_node_id_pool[id]=false;
	--_node_number; 
	delete node;
	return id;
}

const node_t& Master::_open_file(const char* file_path,
		int flag,
		ssize_t& file_no,
		int exist_flag,
		task_parallel_queue<IO_task>* output_queue)throw(std::runtime_error, std::invalid_argument, std::bad_alloc)
{
	file_stat_pool_t::iterator it; 
	open_file_info *file=NULL; 
	//file not buffered
	if(_file_stat_pool.end() ==  (it=_file_stat_pool.find(file_path)))
	{
		file_no=_get_file_no(); 
		if(-1 != file_no)
		{
			Master_file_stat* file_status=_create_new_file_stat(file_path, exist_flag);
			file=_create_new_open_file_info(file_no, flag, file_status, output_queue);
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
		if(NULL == file_stat.opened_file_info)
		{
			file_no=_get_file_no(); 
			file=_create_new_open_file_info(file_no, flag, &file_stat, output_queue);
		}
		else
		{
			file=file_stat.opened_file_info;
			file_no=file->file_no;
		}
		++file->open_count;
	}
	return file->p_node;
}

ssize_t Master::_get_node_id()
{
	if(MAX_NODE_NUMBER == _node_number)
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

void Master::_create_file(const char* file_path, mode_t mode)throw(std::runtime_error)
{
	std::string real_path=_get_real_path(file_path);

	/*int fd=creat64(real_path.c_str(), mode);
	if(-1 == fd)
	{
		throw std::runtime_error("error on create file");
	}
	else
	{
		close(fd);
		return ;
	}*/
}


open_file_info* Master::_create_new_open_file_info(ssize_t file_no,
		int flag,
		Master_file_stat* stat,
		task_parallel_queue<IO_task>* output_queue)throw(std::invalid_argument)
{
	try
	{
		open_file_info *new_file=new open_file_info(file_no, flag, stat); 
		_buffered_files.insert(std::make_pair(file_no, new_file));
		new_file->block_size=_get_block_size(stat->get_status().st_size); 
		stat->opened_file_info=new_file;
		_send_open_request_to_IOnodes(*new_file, output_queue);
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

	/*if(EXISTING == exist_flag)
	{
		if(-1 == stat(real_path.c_str(), &file_status))
		{
			_DEBUG("file can not open\n");
			throw std::invalid_argument("file can not open");
		}
	}
	else
	{*/
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

node_t Master::_select_IOnode(off64_t start_point,
		size_t file_size,
		size_t block_size,
		node_block_map_t& node_block_map)
{
	off64_t block_start_point=_get_block_start_point(start_point, file_size);
	//int node_number = (file_size+block_size-1)/block_size,count=(node_number+_node_number-1)/_node_number, count_node=0;
	node_t nodes;
	if(0 == _node_number)
	{
		return nodes;
	}
	int node_number = (file_size+block_size-1)/block_size,count=(node_number+_node_number-1)/_node_number, count_node=0;
	size_t remaining_size=file_size;
	if(0 == count && 0 == node_number)
	{
		count=1;
		node_number=1;
	}
	if(_registed_IOnodes.end() == _current_IOnode)
	{
		_current_IOnode=_registed_IOnodes.begin();
	}
	for(IOnode_t::const_iterator it=_current_IOnode++;
			_registed_IOnodes.end() != it;++it)
	{
		for(int i=0;i<count && (count_node++)<node_number;++i)
		{
			nodes.insert(std::make_pair(block_start_point, it->first));
			//node_pool.insert(it->first);
			node_block_map[it->first].insert(std::make_pair(block_start_point, MIN(remaining_size, block_size)));
			block_start_point += block_size;
			remaining_size -= block_size;
		}
	}
	return nodes; 
}

node_t& Master::_get_IOnodes_for_IO(off64_t start_point,
		size_t &size,
		struct open_file_info& file,
		node_t& node_set,
		node_id_pool_t& node_id_pool,
		task_parallel_queue<IO_task>* output_queue)throw(std::bad_alloc)
{
	off64_t current_point=_get_block_start_point(start_point, size);
	ssize_t remaining_size=size;
	node_t::iterator it;
	//node_block_map_t node_append_block;
	for(;remaining_size>0;remaining_size-=BLOCK_SIZE, current_point+=BLOCK_SIZE)
	{
		if(file.p_node.end() != (it=file.p_node.find(current_point)))
		{
			node_set.insert(std::make_pair(it->first, it->second));
			node_id_pool.insert(it->second);
		}
		else
		{
			try
			{
				int node_id=_allocate_one_block(file);
				_append_block(file, node_id, current_point);
				node_set.insert(std::make_pair(current_point, node_id));
				node_id_pool.insert(node_id);
				size_t IO_size=MIN(remaining_size, static_cast<ssize_t>(BLOCK_SIZE));
				//node_append_block[node_id].insert(std::make_pair(current_point, IO_size));
			}
			catch(std::bad_alloc& e)
			{
				throw;
			}
		}
	}
	//_send_append_request(file.file_no, node_append_block, output_queue);
	return node_set;
}

int Master::_allocate_one_block(const struct open_file_info& file)throw(std::bad_alloc)
{
	//temp solution
	int node_id=*file.nodes.begin();
	//node_info &selected_node=_registed_IOnodes.at(node_id);
	return node_id;
/*	if(selected_node.avaliable_memory >= BLOCK_SIZE)
	{
		selected_node.avaliable_memory -= BLOCK_SIZE;
		return node_id;
	}
	else
	{
		throw std::bad_alloc();
	}*/
}

void Master::_append_block(struct open_file_info& file, int node_id, off64_t start_point)
{
	file.p_node.insert(std::make_pair(start_point, node_id));
	file.nodes.insert(node_id);
}


int Master::_remove_file(ssize_t file_no, task_parallel_queue<IO_task>* output_queue)
{
	open_file_info &file=*_buffered_files.at(file_no);
	for(node_pool_t::iterator it=file.nodes.begin();
			it != file.nodes.end();++it)
	{
		//int ret;
		IO_task* output=output_queue->allocate_tmp_node();
		output->set_socket(_registed_IOnodes.at(*it)->socket);
		output->push_back(UNLINK);
		_DEBUG("unlink file no=%ld\n", file_no);
		output->push_back(file_no);
		output_queue->task_enqueue();
		//Recv(socket, ret);
	}
	_release_file_no(file_no);
	_file_stat_pool.erase(file.file_status->full_path);
	_buffered_files.erase(file_no);
	delete &file;
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
	_file_stat_pool.rd_lock();
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
	_file_stat_pool.unlock();
	return files;
}

int Master::_buffer_all_meta_data_from_remote(const char* mount_point)throw(std::runtime_error)
{
	char file_path[PATH_MAX];
	DIR *dir=opendir(mount_point);
	if(NULL == dir)
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
	const struct dirent* dir_item=NULL;
	while(NULL != (dir_item=readdir(current_remote_directory)))
	{
		if(0 == strcmp(dir_item->d_name, ".") || 0 == strcmp(dir_item->d_name, ".."))
		{
			continue;
		}
		strcpy(file_path+offset, dir_item->d_name);
		size_t len=strlen(dir_item->d_name);
		file_path[offset+len]=0;
		if(master_number == file_path_hash(std::string(file_path), master_total_size))
		{
			struct stat file_status;
			lstat(file_path, &file_status);
			file_stat_pool_t::iterator it=_file_stat_pool.insert(std::make_pair(file_relative_path, Master_file_stat(file_status, dir_item->d_name, EXISTING))).first;
			it->second.set_file_full_path(it);
		}
		if(DT_DIR == dir_item->d_type)
		{
			DIR* new_dir=opendir(file_path);
			if(NULL == new_dir)
			{
				throw std::runtime_error(file_path);
			}
			file_path[offset+len]='/';
			_dfs_items_in_remote(new_dir, file_path, file_relative_path, offset+len+1);
		}
	}
	return SUCCESS;
}

/*void Master::_send_node_info(int clientfd)const 
{
	_LOG("requery for IOnode info\n");
	Send(clientfd, static_cast<int>(_registed_IOnodes.size()));

	for(IOnode_t::const_iterator it=_registed_IOnodes.begin(); it!=_registed_IOnodes.end(); ++it)
	{
		const node_info *node=it->second;
		Sendv(clientfd, node->ip.c_str(), node->ip.size());
		Send(clientfd, node->total_memory);
		Send_flush(clientfd, node->avaliable_memory);
	}
	return; 
}

void Master::_send_open_file_info(int clientfd)const 
{
	ssize_t file_no=0;
	_LOG("requery for File info\n");
	Recv(clientfd, file_no);

	try
	{
		const open_file_info *const file=_file_stat_pool.at(file_no);
		Send(clientfd, SUCCESS);
		Sendv(clientfd, file->get_path().c_str(), file->get_path().size());
		Send(clientfd, file->get_stat().st_size);
		Send(clientfd, file->block_size);
		Send_flush(clientfd, file->flag);
	}
	catch(std::out_of_range &e)
	{
		Send_flush(clientfd, NO_SUCH_FILE);
	}
	return; 
}

int Master::_send_file_meta(int clientfd)const
{
	char *path=NULL;
	_LOG("requery for File meta data\n");
	struct stat buff;
	Recvv(clientfd, &path);
	try
	{
		//const open_file_info &file=_file_stat_pool.at(file_no);
		Send(clientfd, SUCCESS);
		Sendv_flush(clientfd, &buff, sizeof(struct stat));
	}
	catch(std::out_of_range &e)
	{
		if(-1 == stat(path, &buff))
		{
			Send_flush(clientfd, NO_SUCH_FILE);
		}
		else
		{
			Send(clientfd, SUCCESS);
			Sendv_flush(clientfd, &buff, sizeof(struct stat));
		}
	} free(path); return SUCCESS; }*/ /*void Master::flush_map(file_stat_pool_t& map)const
{
	_DEBUG("start\n");
	for(file_stat_pool_t::const_iterator it=map.begin();
			it!=map.end();++it)
	{
		_DEBUG("key %s\n", it->first.c_str());
		try
		{
			_DEBUG("file no %d\n", it->second.get_fd());
		}
		catch(std::out_of_range &e)
		{}
	}
	_DEBUG("end\n");
}*/
