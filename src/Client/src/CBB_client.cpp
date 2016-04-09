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
#include "CBB_const.h"
//#include "Communication.h"

using namespace CBB::Common;
using namespace CBB::Client;

const char* CBB_client::CLIENT_MOUNT_POINT="CBB_CLIENT_MOUNT_POINT";
const char* CBB_client::MASTER_IP_LIST="CBB_MASTER_IP_LIST";
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
#define CHECK_INIT()                                                      \
	do                                                                \
	{                                                                 \
		if(!_initial)                                             \
		{                                                         \
			start_threads();				  \
		}                                                         \
	}while(0)

CBB_client::CBB_client():
	Client(CLIENT_QUEUE_NUM),
	_fid_now(0),
	_file_list(_file_list_t()),
	_opened_file(_file_t(MAX_FILE)),
	_path_file_meta_map(),
	_client_addr(sockaddr_in()),
	_initial(false),
	master_socket_list(),
	IOnode_fd_map()
{
	_DEBUG("start initalizing\n");
	
	if(nullptr == (mount_point=getenv(CLIENT_MOUNT_POINT)))
	{
		fprintf(stderr, "please set mount point\n");
		throw std::runtime_error("lack of parameter");
	}
	//_DEBUG("%s\n", mount_point);
	memset(&_client_addr, 0, sizeof(_client_addr));
	for(int i=0;i<MAX_FILE; ++i)
	{
		_opened_file[i]=false;
	}
}

void CBB_client::start_threads()
{
	_client_addr.sin_family = AF_INET;
	_client_addr.sin_port = htons(0);
	_client_addr.sin_addr.s_addr = htons(INADDR_ANY);
	Client::start_client();
	if(-1 == _regist_to_master())
	{
		return;
	}
	_DEBUG("initialization finished\n");
	_initial=true;
}

int CBB_client::_regist_to_master()
{
	int ret=0;
	struct sockaddr_in _master_addr;
	const char* master_ip_list=getenv(MASTER_IP_LIST), *next_master_ip=nullptr;
	char master_ip[20];
	if(nullptr == master_ip_list)
	{
		fprintf(stderr, "please set master ip\n");
		return -1;
	}
	memset(&_master_addr, 0, sizeof(_master_addr));
	_master_addr.sin_family = AF_INET;
	_master_addr.sin_port = htons(MASTER_PORT);
	while(0 != *master_ip_list)
	{
		next_master_ip=strchr(master_ip_list, ',');
		if(nullptr == next_master_ip)
		{
			for(next_master_ip=master_ip_list;
					'\0' != *next_master_ip; ++ next_master_ip);
			strncpy(master_ip, master_ip_list, next_master_ip-master_ip_list);
			master_ip[next_master_ip-master_ip_list]=0;
			master_ip_list=next_master_ip;
		}
		else
		{
			strncpy(master_ip, master_ip_list, next_master_ip-master_ip_list);
			master_ip[next_master_ip-master_ip_list]=0;
			master_ip_list=next_master_ip+1;
		}

		if(0 == inet_aton(master_ip, &_master_addr.sin_addr))
		{
			perror("Master IP Address Error");
			_DEBUG("%s\n", master_ip);
			return -1;
		}
		int master_socket=Client::_connect_to_server(_client_addr, _master_addr); 
		CBB_communication_thread::_add_socket(master_socket);
		master_socket_list.push_back(master_socket);
		extended_IO_task* query=allocate_new_query(master_socket);
		query->push_back(NEW_CLIENT);
		send_query(query);

		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		response_dequeue(response);
	}
	return ret;
}

CBB_client::~CBB_client()
{
	for(_file_list_t::iterator it=_file_list.begin();
			it!=_file_list.end();++it)
	{
		int ret=0;
		extended_IO_task* query=allocate_new_query(it->second.file_meta_p->master_socket);
		query->push_back(CLOSE_FILE);
		query->push_back(it->second.file_meta_p->file_no);
		send_query(query);

		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		response_dequeue(response);
		release_communication_queue(query);
	}
	for(master_list_t::iterator it=master_socket_list.begin();
			it != master_socket_list.end();++it)
	{
		extended_IO_task* query=allocate_new_query(*it);
		query->push_back(CLOSE_CLIENT);
		send_query(query);
		release_communication_queue(query);
	}
	for(IOnode_fd_map_t::iterator it=IOnode_fd_map.begin();
			it!=IOnode_fd_map.end();++it)
	{
		extended_IO_task* query=allocate_new_query(it->second);
		query->push_back(CLOSE_CLIENT);
		send_query(query);
		release_communication_queue(query);
	}
}

/*CBB_client::block_info::block_info(ssize_t node_id, off64_t start_point, size_t size):
	node_id(node_id),
	start_point(start_point),
	size(size)
{}*/

CBB_client::block_info::block_info(off64_t start_point, size_t size):
	start_point(start_point),
	size(size)
{}

CBB_client::file_meta::file_meta(ssize_t file_no,
		size_t block_size,
		const struct stat* file_stat,
		int master_number,
		int master_socket):
	file_no(file_no),
	open_count(0),
	block_size(block_size),
	file_stat(*file_stat),
	opened_fd(),
	master_number(master_number),
	master_socket(master_socket),
	it(nullptr)
{}

CBB_client::opened_file_info::opened_file_info(int fd,
		int flag,
		file_meta* file_meta_p):
	current_point(0),
	fd(fd),
	flag(flag),
	file_meta_p(file_meta_p)
{
	++file_meta_p->open_count;
}

CBB_client::opened_file_info::opened_file_info():
	current_point(0),
	fd(0),
	flag(-1),
	file_meta_p(nullptr)
{}

CBB_client::opened_file_info::~opened_file_info()
{
	if(nullptr != file_meta_p && 0 == --file_meta_p->open_count)
	{
		delete file_meta_p;
	}
}

CBB_client::opened_file_info::opened_file_info(const opened_file_info& src):
	current_point(src.current_point),
	fd(src.fd),
	flag(src.flag),
	file_meta_p(src.file_meta_p)
{
	++file_meta_p->open_count;
}

void CBB_client::_get_blocks_from_master(extended_IO_task* response,
		off64_t start_point,
		size_t& size,
		_block_list_t &block,
		_node_pool_t &node_pool)
{
	char *ip=nullptr;
	
	int count=0;
	ssize_t node_id=0;
	size_t block_size=0;

	response->pop(count);
	for(int i=0;i<count;++i)
	{
		response->pop(node_id);
		response->pop_string(&ip);
		node_pool.insert(std::make_pair(node_id, std::string(ip)));
	}
	response->pop(count);
	for(int i=0;i<count;++i)
	{
		response->pop(start_point);
		//response->pop(node_id);
		response->pop(block_size);
		block.push_back(block_info(start_point, block_size));
	}
	return;
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

int CBB_client::_open(const char * path, int flag, mode_t mode)throw(std::runtime_error)
{
	CHECK_INIT();
	file_meta* file_meta_p=nullptr;
	std::string string_path=std::string(path);
	int master_number=_get_master_number_from_path(path);
	int master_socket=_get_master_socket_from_master_number(master_number);
	int ret=master_number;
	int fid=0;
	extended_IO_task* response=nullptr;
	if(-1 == (fid=_get_fid()))
	{
		errno = EMFILE;
		return -1; 
	}
	try
	{
		file_meta_p=_path_file_meta_map.at(string_path);
		/*if(-1 == file_meta_p->file_no)
		{
			throw std::out_of_range("");
		}*/
	}
	catch(std::out_of_range& e)
	{
		_DEBUG("connect to master\n");
		do{
			master_number = ret;
			extended_IO_task* query=allocate_new_query(master_socket);
			query->push_back(OPEN_FILE); 
			query->push_back_string(path);

			if(flag & O_CREAT)
			{
				query->push_back(flag); 
			}
			query->push_back(mode);
			send_query(query);
			response=get_query_response(query);
			master_socket = _get_master_socket_from_master_number(ret);
		}while(master_number != ret && response_dequeue(response));
		response->pop(ret);
		if(SUCCESS == ret)
		{
			_DEBUG("open finished\n");
			if(nullptr == file_meta_p)
			{
				file_meta_p=_create_new_file(response, master_number);
				_path_file_meta_map_t::iterator it=_path_file_meta_map.insert(std::make_pair(string_path, file_meta_p)).first;
				file_meta_p->it=it;
			}
			else
			{
				response->pop(file_meta_p->file_no);
				response->pop(file_meta_p->block_size);
				Recv_attr(response, &file_meta_p->file_stat);
			}
		}
		else
		{
			//Recv(master_socket, errno);
			errno=-ret;
			response_dequeue(response);
			return -1;
		}
		response_dequeue(response);
	}
	int fd=_fid_to_fd(fid);
	opened_file_info& file=_file_list.insert(std::make_pair(fid, opened_file_info(fd, flag, file_meta_p))).first->second;
	file_meta_p->opened_fd.insert(fd);
	if(flag & O_APPEND)
	{
		file.current_point=file_meta_p->file_stat.st_size;
	}
	_DEBUG("file no =%ld, fid = %d\n", file_meta_p->file_no, _fid_to_fd(fid));
	return fd;
}

CBB_client::file_meta* CBB_client::_create_new_file(extended_IO_task* response, int master_number)
{
	ssize_t file_no=0;
	size_t block_size=0;
	struct stat file_stat;
	response->pop(file_no);
	response->pop(block_size);
	Recv_attr(response, &file_stat);
	file_meta* file_meta_p=new file_meta(file_no, block_size, &file_stat, master_number, response->get_socket());
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

ssize_t CBB_client::_read_from_IOnode(int master_number,
		opened_file_info& file,
		const _block_list_t& blocks,
		const _node_pool_t& node_pool,
		char* buffer,
		size_t size)
{
	ssize_t ans=0;
	int ret=0;
	off64_t current_point=file.current_point;
	size_t read_size=size;
	ssize_t node_id=begin(node_pool)->first;
	off64_t start_point=begin(blocks)->start_point;
	const std::string& node_ip=begin(node_pool)->second;
	extended_IO_task *response=nullptr;
	int IOnode_socket=_get_IOnode_socket(master_number, node_id, node_ip);

	while(0 < read_size)
	{
		off64_t offset = current_point-start_point;
		size_t IO_size = MAX_TRANSFER_SIZE > read_size? read_size : MAX_TRANSFER_SIZE;

		do
		{
			extended_IO_task* query=allocate_new_query(IOnode_socket);
			query->push_back(READ_FILE);
			query->push_back(file.file_meta_p->file_no);
			query->push_back(start_point);
			query->push_back(offset);
			query->push_back(IO_size);
			send_query(query);

			response=get_query_response(query);
			response->pop(ret);
		}
		while(FILE_NOT_FOUND == ret && 0 == response_dequeue(response));
		if(SUCCESS == ret)
		{
			ssize_t ret_size=response->get_received_data_all(buffer);

			_DEBUG("IO_size=%lu, start_point=%ld, file_no=%ld, ret_size=%ld\n", IO_size, start_point, file.file_meta_p->file_no, ret_size);
			if(-1 == ret_size)
			{
				response_dequeue(response);
				return ans;
			}
			else
			{
				_DEBUG("read size=%ld IOnode ip=%s current point=%ld\n", ret_size, node_ip.c_str(), file.current_point);
				ans+=ret_size;
				file.current_point += ret_size;
				current_point += ret_size;
				start_point=find_start_point(blocks, start_point, ret_size);
				read_size -= ret_size;
				buffer+=ret_size;
			}
		}
		//*buffer=0;
		response_dequeue(response);
	}
	return ans;
}

ssize_t CBB_client::_write_to_IOnode(int master_number,
		opened_file_info& file,
		const _block_list_t& blocks,
		const _node_pool_t& node_pool,
		const char* buffer,
		size_t size)
{
	ssize_t ans=0;
	int ret=0;
	off64_t current_point=file.current_point;
	size_t write_size=size;
	ssize_t node_id=begin(node_pool)->first;
	off64_t start_point=begin(blocks)->start_point;
	const std::string& node_ip=begin(node_pool)->second;
	int IOnode_socket=_get_IOnode_socket(master_number, node_id, node_ip);
	extended_IO_task* response=nullptr;

	while(0 < write_size)
	{

		off64_t offset = current_point-start_point;
		size_t IO_size = MAX_TRANSFER_SIZE > write_size? write_size: MAX_TRANSFER_SIZE;

		_DEBUG("IO_size=%lu, start_point=%ld, file_no=%ld\n", IO_size, start_point, file.file_meta_p->file_no);
		do
		{
			extended_IO_task* query=allocate_new_query(IOnode_socket);
			query->push_back(WRITE_FILE);
			query->push_back(file.file_meta_p->file_no);
			query->push_back(start_point);
			query->push_back(offset);
			query->push_back(IO_size);
			query->push_send_buffer(buffer, IO_size);
			send_query(query);

			response=get_query_response(query);
			response->pop(ret);
		}
		while(FILE_NOT_FOUND == ret && 0 == response_dequeue(response));
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
				_DEBUG("write size=%ld IOnode ip=%s current point=%ld\n", ret_size, node_ip.c_str(), file.current_point);
				ans+=ret_size;
				file.current_point+=ret_size;
				current_point += ret_size;
				start_point=find_start_point(blocks, start_point, ret_size);
				buffer+=ret_size;
				write_size -= ret_size;
			}
		}
		response_dequeue(response);
	}	
	return ans;
}

int CBB_client::_get_IOnode_socket(int master_number, ssize_t IOnode_id, const std::string& ip)
{
	ssize_t master_IOnode_id=_get_IOnode_id(master_number, IOnode_id);
	try
	{
		return IOnode_fd_map.at(master_IOnode_id);
	}
	catch(std::out_of_range& e)
	{
		struct sockaddr_in IOnode_addr;
		set_server_addr(ip, IOnode_addr);
		int IOnode_socket=Client::_connect_to_server(_client_addr, IOnode_addr);
		CBB_communication_thread::_add_socket(IOnode_socket);
		int ret=FAILURE;
		extended_IO_task* query=allocate_new_query(IOnode_socket);
		query->push_back(NEW_CLIENT);
		send_query(query);
		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		response_dequeue(response);
		if(SUCCESS == ret)
		{
			IOnode_fd_map.insert(std::make_pair(master_IOnode_id, IOnode_socket));
			return IOnode_socket;
		}
		else
		{
			CBB_communication_thread::_delete_socket(IOnode_socket);
			close(IOnode_socket);
			return -1;
		}
	}
}

ssize_t CBB_client::_read(int fd, void *buffer, size_t size)throw(std::runtime_error)
{
	int ret=0;
	CHECK_INIT();
	if(0 == size)
	{
		return size;
	}
	int fid=_fd_to_fid(fd);
	try
	{
		opened_file_info& file=_file_list.at(fid);
		ssize_t file_no=file.file_meta_p->file_no;
		_block_list_t blocks;
		_node_pool_t node_pool;

		if(size+file.current_point > (size_t)file.file_meta_p->file_stat.st_size)
		{
			if(file.file_meta_p->file_stat.st_size>file.current_point)
			{
				size=file.file_meta_p->file_stat.st_size-file.current_point;
			}
			else
			{
				return 0;
			}
		}
		int master_socket=_get_master_socket_from_fd(fid);
		int master_number=_get_master_number_from_fd(fid);
		extended_IO_task* query=allocate_new_query(master_socket);
		query->push_back(READ_FILE);
		query->push_back(file_no);
		query->push_back(file.current_point);
		query->push_back(size);
		send_query(query);

		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		if(SUCCESS == ret)
		{
			_update_file_size_from_master(response, file);
			_get_blocks_from_master(response, file.current_point, size, blocks, node_pool);
			response_dequeue(response);
			ret=_read_from_IOnode(master_number, file, blocks, node_pool, static_cast<char *>(buffer), size);
		}
		else
		{
			errno=-ret;
			ret=-1;
			response_dequeue(response);
		}
		return ret;

	}
	catch(std::out_of_range &e)
	{
		errno = EBADFD;
		return -1;
	}
}

ssize_t CBB_client::_write(int fd, const void *buffer, size_t size)throw(std::runtime_error)
{
	int ret=0;
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	if(0 == size)
	{
		return size;
	}
	try
	{
		opened_file_info& file=_file_list.at(fid);
		ssize_t file_no=file.file_meta_p->file_no;
		off64_t &current_point=file.current_point;
		_write_update_file_size(file, size);
		_update_access_time(fd);

		_block_list_t blocks;
		_node_pool_t node_pool;

		int master_socket=_get_master_socket_from_fd(fid);
		int master_number=_get_master_number_from_fd(fid);
		extended_IO_task* query=allocate_new_query(master_socket);
		query->push_back(WRITE_FILE);
		query->push_back(file_no);
		Send_attr(query, &file.file_meta_p->file_stat);
		query->push_back(file.current_point);
		query->push_back(size);

		send_query(query);

		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		if(SUCCESS == ret)
		{
			_update_file_size_from_master(response, file);

			_get_blocks_from_master(response, current_point, size, blocks, node_pool);
			response_dequeue(response);
			ret=_write_to_IOnode(master_number, file, blocks, node_pool,  static_cast<const char *>(buffer), size);
		}
		else
		{
			errno=-ret;
			ret=-1;
			response_dequeue(response);
		}
	}
	catch(std::out_of_range &e)
	{
		errno = EBADFD;
		ret=-1;
	}
	return ret;

}

size_t CBB_client::_update_file_size_from_master(extended_IO_task* response, opened_file_info& file)
{
	size_t file_size;
	response->pop(file_size);
	/*if(file_size > file.file_meta_p->file_stat.st_size)
	{
		file.file_meta_p->file_stat.st_size = file_size;
	}*/
	return file_size;
}

int CBB_client::_write_update_file_size(opened_file_info& file, size_t size)
{
	if(file.current_point + size > (size_t)file.file_meta_p->file_stat.st_size)
	{
		file.file_meta_p->file_stat.st_size=file.current_point + size;
		return SUCCESS;
	}
	return FAILURE;
}

int CBB_client::_update_file_size(int fd, size_t size)
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

int CBB_client::_update_access_time(int fd)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	try
	{
		opened_file_info& file=_file_list.at(fid);
		time_t new_time=time(nullptr);
		file.file_meta_p->file_stat.st_mtime=new_time;
		file.file_meta_p->file_stat.st_atime=file.file_meta_p->file_stat.st_mtime;
		return 0;
	}
	catch(std::out_of_range&e)
	{
		errno=EBADFD;
		return -1;
	}
}

int CBB_client::_close(int fd)throw(std::runtime_error)
{
	int ret=0;
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	int master_socket=_get_master_socket_from_fd(fid);
	try
	{
		opened_file_info& file=_file_list.at(fid);
		extended_IO_task* query=allocate_new_query(master_socket);
		query->push_back(CLOSE_FILE);
		query->push_back(file.file_meta_p->file_no);
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

int CBB_client::_flush(int fd)throw(std::runtime_error)
{
	int ret=0;
	int fid=_fd_to_fid(fd);
	int master_socket=_get_master_socket_from_fd(fid);
	CHECK_INIT();
	try
	{
		opened_file_info& file=_file_list.at(fid);
		extended_IO_task* query=allocate_new_query(master_socket);
		query->push_back(FLUSH_FILE);
		query->push_back(file.file_meta_p->file_no);
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

off64_t CBB_client::_lseek(int fd, off64_t offset, int whence)
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

int CBB_client::_getattr(const char* path, struct stat* fstat)throw(std::runtime_error)
{
	CHECK_INIT();
	int master_number=_get_master_number_from_path(path);
	int master_socket=_get_master_socket_from_master_number(master_number);
	int ret=master_number;
	extended_IO_task* response=nullptr;

	if(SUCCESS == _get_local_attr(path, fstat))
	{
		_DEBUG("use local stat\n");
		return 0;
	}
	else
	{
		_DEBUG("connect to master\n");
		do{
			//problem
			master_number=ret;
			extended_IO_task* query=allocate_new_query(master_socket);
			query->push_back(GET_ATTR); 
			query->push_back_string(path);
			send_query(query);
			
			response=get_query_response(query);
			response->pop(ret);
			master_socket = _get_master_socket_from_master_number(ret);
		}while(master_number != ret && response_dequeue(response));
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
}

int CBB_client::_readdir(const char * path, dir_t& dir)throw(std::runtime_error)

{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();

	for(master_list_t::const_iterator it=master_socket_list.begin();
			it != master_socket_list.end(); ++it)
	{
		int master_socket=*it;
		extended_IO_task* query=allocate_new_query(master_socket);
		query->push_back(READ_DIR); 
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
			for(int i=0; i<count; ++i)
			{
				response->pop_string(&path);
				dir.insert(std::string(path));
			}
			//Recv(master_socket, count);
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

int CBB_client::_rmdir(const char * path)throw(std::runtime_error)

{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int master_socket=_get_master_socket_from_path(path);

	extended_IO_task* query=allocate_new_query(master_socket);
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

int CBB_client::_unlink(const char * path)throw(std::runtime_error)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	_close_local_opened_file(path);
	int master_socket=_get_master_socket_from_path(path);

	extended_IO_task* query=allocate_new_query(master_socket);
	query->push_back(UNLINK); 
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

int CBB_client::_close_local_opened_file(const char* path)
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

int CBB_client::_access(const char* path, int mode)throw(std::runtime_error)
{
	_DEBUG("connect to master\n");
	CHECK_INIT();
	extended_IO_task* response=nullptr;
	std::string string_path(path);

	auto file_meta=_path_file_meta_map.find(string_path);
	if(end(_path_file_meta_map) != file_meta)
	{
		return 0;
	}
	else
	{
		int master_number=_get_master_number_from_path(path);
		int master_socket=_get_master_socket_from_master_number(master_number);
		int ret=master_number;
		do{
			master_number=ret;
			extended_IO_task* query=allocate_new_query(master_socket);
			query->push_back(ACCESS); 
			query->push_back_string(path);
			query->push_back(mode);
			send_query(query);
	
			response=get_query_response(query);
			response->pop(ret);
			master_socket = _get_master_socket_from_master_number(ret);
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
}

int CBB_client::_stat(const char* path, struct stat* buf)throw(std::runtime_error)
{
	CHECK_INIT();
	memset(buf, 0, sizeof(struct stat));
	int master_number=_get_master_number_from_path(path);
	int master_socket=_get_master_socket_from_master_number(master_number);
	int ret=master_number;
	extended_IO_task* response=nullptr;

	do{
		master_number=ret;
		extended_IO_task* query=allocate_new_query(master_socket);
		query->push_back(GET_FILE_META);
		query->push_back_string(path);
		send_query(query);
		
		extended_IO_task* response=get_query_response(query);
		response->pop(ret);
		master_socket = _get_master_socket_from_master_number(ret);
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

int CBB_client::_rename(const char* old_name, const char* new_name)throw(std::runtime_error)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int new_master_number=_get_master_number_from_path(new_name), old_master_number=_get_master_number_from_path(old_name);
	int old_master_socket=_get_master_socket_from_master_number(old_master_number), new_master_socket=_get_master_socket_from_master_number(new_master_number);

	_path_file_meta_map_t::iterator it=_path_file_meta_map.find(old_name);
	extended_IO_task* response=nullptr;

	if(_path_file_meta_map.end() != it)
	{
		
		_path_file_meta_map_t::iterator new_it=_path_file_meta_map.insert(std::make_pair(new_name, it->second)).first;
		_path_file_meta_map.erase(it);
		new_it->second->it=new_it;
	}

	extended_IO_task* query=allocate_new_query(old_master_socket);
	query->push_back(RENAME); 
	query->push_back_string(old_name);
	query->push_back_string(new_name);

	if(new_master_number == old_master_number)
	{
		query->push_back(MYSELF);
		send_query(query);
		response=get_query_response(query);
		response->pop(ret);
	}
	else
	{
		query->push_back(new_master_number);
		send_query(query);
		response=get_query_response(query);
		response->pop(ret);
		response_dequeue(response);

		extended_IO_task* query=allocate_new_query(new_master_socket);
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

int CBB_client::_mkdir(const char* path, mode_t mode)throw(std::runtime_error)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int master_socket=_get_master_socket_from_path(path);

	extended_IO_task* query=allocate_new_query(master_socket);
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

off64_t CBB_client::_tell(int fd)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	return _file_list.at(fid).current_point;
}

bool CBB_client::_interpret_path(const char *path)
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

bool CBB_client::_interpret_fd(int fd)
{
	if(fd<INIT_FD)
	{
		return false;
	}
	return true;
}

void CBB_client::_format_path(const char *path, std::string &formatted_path)
{
	char *real_path=static_cast<char *>(malloc(PATH_MAX));
	realpath(path, real_path);

	formatted_path=std::string(real_path);
	free(real_path);
	return;
}

void CBB_client::_get_relative_path(const std::string &formatted_path, std::string &relative_path)
{
	const char *pointer=formatted_path.c_str()+strlen(mount_point);
	relative_path=std::string(pointer);
	return;
}

size_t CBB_client::_get_file_size(int fd)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	return _file_list.at(fid).file_meta_p->file_stat.st_size;
}

int CBB_client::_get_local_attr(const char* path, struct stat *file_stat)
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

int CBB_client::_truncate(const char* path, off64_t size)throw(std::runtime_error)
{
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int master_number=_get_master_number_from_path(path);
	int master_socket=_get_master_socket_from_master_number(master_number);
	int ret=master_number;
	extended_IO_task* response=nullptr;

	do{
		master_number=ret;
		extended_IO_task* query=allocate_new_query(master_socket);
		query->push_back(TRUNCATE); 
		query->push_back_string(path);
		query->push_back(size);
		send_query(query);
		
		response=get_query_response(query);
		response->pop(ret);
		master_socket = _get_master_socket_from_master_number(ret);
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

int CBB_client::_ftruncate(int fd, off64_t size)throw(std::runtime_error)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	opened_file_info& file=_file_list.at(fid);
	file.file_meta_p->file_stat.st_size=size;
	_update_access_time(fd);
	return _truncate(file.file_meta_p->it->first.c_str(), size);
}

int CBB_client::open(const char *path, int flag, mode_t mode)
{
	int ret=0;
	do
	{
		try
		{
			ret=_open(path, flag, mode);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

ssize_t CBB_client::read(int fd, void *buffer, size_t size)
{
	ssize_t ret=0;
	do
	{
		try
		{
			ret=_read(fd, buffer, size);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

ssize_t CBB_client::write(int fd,const void *buffer, size_t size)
{
	ssize_t ret=0;
	do
	{
		try
		{
			ret=_write(fd, buffer, size);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

int CBB_client::close(int fd)
{
	int ret=0;
	do
	{
		try
		{
			ret=_close(fd);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

int CBB_client::flush(int fd)
{
	int ret=0;
	do
	{
		try
		{
			ret=_flush(fd);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

off64_t CBB_client::lseek(int fd, off64_t offset, int whence)
{
	return _lseek(fd, offset, whence);
}

int CBB_client::getattr(const char *path, struct stat* fstat)
{
	int ret=0;
	do
	{
		try
		{
			ret=_getattr(path, fstat);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

int CBB_client::readdir(const char* path, dir_t& dir)
{
	int ret=0;
	do
	{
		try
		{
			ret=_readdir(path, dir);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

int CBB_client::unlink(const char* path)
{
	int ret=0;
	do
	{
		try
		{
			ret=_unlink(path);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

int CBB_client::rmdir(const char* path)
{
	int ret=0;
	do
	{
		try
		{
			ret=_rmdir(path);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

int CBB_client::access(const char* path, int mode)
{
	int ret=0;
	do
	{
		try
		{
			ret=_access(path, mode);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

int CBB_client::stat(const char* path, struct stat* buf)
{
	int ret=0;
	do
	{
		try
		{
			ret=_stat(path, buf);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

int CBB_client::rename(const char* old_name, const char* new_name)
{
	int ret=0;
	do
	{
		try
		{
			ret=_rename(old_name, new_name);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

int CBB_client::mkdir(const char* path, mode_t mode)
{
	int ret=0;
	do
	{
		try
		{
			ret=_mkdir(path, mode);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

int CBB_client::truncate(const char*path, off64_t size)
{
	int ret=0;
	do
	{
		try
		{
			ret=_truncate(path, size);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

int CBB_client::ftruncate(int fd, off64_t size)
{
	int ret=0;
	do
	{
		try
		{
			ret=_ftruncate(fd, size);
			break;
		}
		catch(std::runtime_error &e)
		{
			_DEBUG("socket error, retry\n");
		}
	}
	while(true);
	return ret;
}

off64_t CBB_client::tell(int fd)
{
	return _tell(fd);
}

int CBB_client::_report_IOnode_failure(int socket)
{
	ssize_t master_IOnode_id=_find_master_IOnode_id_by_socket(socket);
	if(-1 == master_IOnode_id)
	{
		return FAILURE;
	}
	int master_number=0;
	ssize_t IOnode_id=0;
	_parse_master_IOnode_id(master_IOnode_id, master_number, IOnode_id);
	int master_socket=master_socket_list.at(master_number);
	_DEBUG("report IOnode failure to master %d, IOnode id %ld\n", master_number, IOnode_id);

	extended_IO_task* query=allocate_new_query(master_socket);
	query->push_back(IONODE_FAILURE);
	query->push_back(IOnode_id);
	send_query(query);

	return SUCCESS;
}

ssize_t CBB_client::_find_master_IOnode_id_by_socket(int socket)
{
	for(auto& IOnode:IOnode_fd_map)
	{
		if(socket == IOnode.second)
		{
			return IOnode.first;
		}
	}
	return -1;
}

off64_t CBB_client::find_start_point(const _block_list_t& blocks, off64_t start_point, ssize_t update_size)
{
	off64_t ret=begin(blocks)->start_point;
	off64_t end_start_point=start_point + update_size;
	for(const auto& block:blocks)
	{
		if( end_start_point < block.start_point )
		{
			return ret;
		}
		else
		{
			ret=block.start_point;
		}
	}
	return ret;
}
