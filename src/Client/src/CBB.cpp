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
#include <functional>

#include "CBB.h"
#include "CBB_internal.h"
#include "CBB_const.h"
#include "Communication.h"

const char* CBB::CLIENT_MOUNT_POINT="CBB_CLIENT_MOUNT_POINT";
const char* CBB::MASTER_IP_LIST="CBB_MASTER_IP_LIST";

const char *mount_point=NULL;

CBB::CBB():
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
	
	if(NULL == (mount_point=getenv(CLIENT_MOUNT_POINT)))
	{
		fprintf(stderr, "please set mount point\n");
		return;
	}
	//_DEBUG("%s\n", mount_point);
	memset(&_client_addr, 0, sizeof(_client_addr));
	for(int i=0;i<MAX_FILE; ++i)
	{
		_opened_file[i]=false;
	}

	_client_addr.sin_family = AF_INET;
	_client_addr.sin_port = htons(0);
	_client_addr.sin_addr.s_addr = htons(INADDR_ANY);
	if(-1 == _regist_to_master())
	{
		return;
	}
	_DEBUG("initialization finished\n");
	_initial=true;
}

inline int CBB::_regist_to_master()
{
	int ret;
	struct sockaddr_in _master_addr;
	const char* master_ip_list=getenv(MASTER_IP_LIST), *next_master_ip=NULL;
	char master_ip[20];
	if(NULL == master_ip_list)
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
		if(NULL == next_master_ip)
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
		Send_flush(master_socket, NEW_CLIENT);
		Recv(master_socket, ret);
		master_socket_list.push_back(master_socket);
	}
	return ret;
}

CBB::~CBB()
{
	for(_file_list_t::iterator it=_file_list.begin();
			it!=_file_list.end();++it)
	{
		int master_socket=it->second.file_meta_p->master_socket;
		int ret=0;
		Send(master_socket, CLOSE_FILE);
		Send(master_socket, it->second.file_meta_p->file_no);
		Recv(master_socket, ret);
	}
	for(master_list_t::iterator it=master_socket_list.begin();
			it != master_socket_list.end();++it)
	{
		Send(*it, CLOSE_CLIENT);
		close(*it);
	}
	for(IOnode_fd_map_t::iterator it=IOnode_fd_map.begin();
			it!=IOnode_fd_map.end();++it)
	{
		int ret=0;
		Send(it->second, CLOSE_CLIENT);
		Recv(it->second, ret);
		close(it->second);
	}
}

CBB::block_info::block_info(ssize_t node_id, off64_t start_point, size_t size):
	node_id(node_id),
	start_point(start_point),
	size(size)
{}

CBB::file_meta::file_meta(ssize_t file_no,
		size_t block_size,
		const struct stat* file_stat,
		int master_socket):
	file_no(file_no),
	open_count(0),
	block_size(block_size),
	file_stat(*file_stat),
	opened_fd(),
	master_socket(master_socket),
	it(NULL)
{}

CBB::opened_file_info::opened_file_info(int fd,
		int flag,
		file_meta* file_meta_p):
	current_point(0),
	fd(fd),
	flag(flag),
	file_meta_p(file_meta_p)
{
	++file_meta_p->open_count;
}

CBB::opened_file_info::opened_file_info():
	current_point(0),
	fd(0),
	flag(-1),
	file_meta_p(NULL)
{}

CBB::opened_file_info::~opened_file_info()
{
	if(NULL != file_meta_p && 0 == --file_meta_p->open_count)
	{
		delete file_meta_p;
	}
}

CBB::opened_file_info::opened_file_info(const opened_file_info& src):
	current_point(src.current_point),
	fd(src.fd),
	flag(src.flag),
	file_meta_p(src.file_meta_p)
{
	++file_meta_p->open_count;
}

void CBB::_get_blocks_from_master(int socket, off64_t start_point, size_t& size, std::vector<block_info> &block, _node_pool_t &node_pool)
{
	char *ip=NULL;
	
	int count=0;
	ssize_t node_id=0;

	Send(socket, start_point);
	Send_flush(socket, size);
	Recv(socket, count);
	for(int i=0;i<count;++i)
	{
		Recv(socket,node_id);
		Recvv(socket, &ip);
		node_pool.insert(std::make_pair(node_id, std::string(ip)));
		delete[] ip;
	}
	Recv(socket, count);
	for(int i=0;i<count;++i)
	{
		Recv(socket, start_point);
		Recv(socket, node_id);
		block.push_back(block_info(node_id, start_point, BLOCK_SIZE));
	}
	Recv(socket, count);
	return;
}
	

int CBB::_get_fid()
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

int CBB::_open(const char * path, int flag, mode_t mode)
{
	int ret=0;
	CHECK_INIT();
	file_meta* file_meta_p=NULL;
	std::string string_path=std::string(path);
	int master_socket=_get_master_socket_from_path(path);
	int fid;
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
		Send(master_socket, OPEN_FILE); 
		Sendv(master_socket, path, strlen(path));
		if(flag & O_CREAT)
		{
			Send(master_socket, flag); 
			Send_flush(master_socket, mode);
		}
		else
		{
			Send_flush(master_socket, flag);
		}
		Recv(master_socket, ret);
		if(SUCCESS == ret)
		{
			_DEBUG("open finished\n");
			if(NULL == file_meta_p)
			{
				file_meta_p=_create_new_file(master_socket);
				_path_file_meta_map_t::iterator it=_path_file_meta_map.insert(std::make_pair(string_path, file_meta_p)).first;
				file_meta_p->it=it;
			}
			else
			{
				Recv(master_socket, file_meta_p->file_no);
				Recv(master_socket, file_meta_p->block_size);
				Recv_attr(master_socket, &file_meta_p->file_stat);
			}
		}
		else
		{
			Recv(master_socket, errno);
			return -1;
		}
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

CBB::file_meta* CBB::_create_new_file(int master_socket)
{
	ssize_t file_no;
	size_t block_size;
	struct stat file_stat;
	Recv(master_socket, file_no);
	Recv(master_socket, block_size);
	Recv_attr(master_socket, &file_stat);
	file_meta* file_meta_p=new file_meta(file_no, block_size, &file_stat, master_socket);
	if(NULL != file_meta_p)
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

ssize_t CBB::_read_from_IOnode(opened_file_info& file, const _block_list_t& blocks, const _node_pool_t& node_pool, char* buffer, size_t size)
{
	ssize_t ans=0;
	int ret=0;
	off64_t current_point=file.current_point;
	size_t read_size=size;
	if(0 == size)
	{
		return size;
	}
	
	for(_block_list_t::const_iterator it=blocks.begin();
			it!=blocks.end();++it)
	{

		off64_t offset = current_point-it->start_point;
		size_t IO_size = BLOCK_SIZE-offset;
		if(read_size<IO_size)
		{
			IO_size=read_size;
		}

		int IOnode_socket=_get_IOnode_socket(it->node_id, node_pool.at(it->node_id));
		Send(IOnode_socket, READ_FILE);
		Send(IOnode_socket, file.file_meta_p->file_no);
		Send(IOnode_socket, it->start_point);
		Send(IOnode_socket, offset);
		Send_flush(IOnode_socket, IO_size);
		Recv(IOnode_socket, ret);
		if(SUCCESS == ret)
		{
			_DEBUG("IO_size=%lu, start_point=%ld, file_no=%ld\n", IO_size, it->start_point, file.file_meta_p->file_no);
			ssize_t ret_size=Recvv_pre_alloc(IOnode_socket, buffer, IO_size);
			if(-1 == ret_size)
			{
				return ans;
			}
			else
			{
				_DEBUG("read size=%ld\n", ret_size);
				_DEBUG("IOnode ip=%s\n", node_pool.at(it->node_id).c_str()); 
				ans+=ret_size;
				file.current_point += ret_size;
				current_point += ret_size;
				read_size -= ret_size;
				buffer+=ret_size;
				_DEBUG("current point=%ld\n", file.current_point);
			}
		}
		*buffer=0;
	}
	return ans;
}

ssize_t CBB::_write_to_IOnode(opened_file_info& file, const _block_list_t& blocks, const _node_pool_t& node_pool, const char* buffer, size_t size)
{
	ssize_t ans=0;
	int ret=0;
	off64_t current_point=file.current_point;
	size_t write_size=size;
	if(0 == size)
	{
		return size;
	}
	for(_block_list_t::const_iterator it=blocks.begin();
			it!=blocks.end();++it)
	{

		off64_t offset = current_point-it->start_point;
		size_t IO_size = BLOCK_SIZE-offset;
		if(write_size<IO_size)
		{
			IO_size=write_size;
		}

		int IOnode_socket=_get_IOnode_socket(it->node_id, node_pool.at(it->node_id));
		Send(IOnode_socket, WRITE_FILE);
		Send(IOnode_socket, file.file_meta_p->file_no);
		Send(IOnode_socket, it->start_point);
		Send(IOnode_socket, offset);
		Send_flush(IOnode_socket, IO_size);
		Recv(IOnode_socket, ret);
		if(SUCCESS == ret)
		{
			_DEBUG("IO_size=%lu, start_point=%ld, file_no=%ld\n", IO_size, it->start_point, file.file_meta_p->file_no);
			ssize_t ret_size=Sendv_pre_alloc_flush(IOnode_socket, buffer, IO_size);
			if(-1 == ret_size)
			{
				return ans;
			}
			else
			{
				_DEBUG("write size=%ld\n", ret_size);
				_DEBUG("IOnode ip=%s\n", node_pool.at(it->node_id).c_str()); 
				ans+=ret_size;
				file.current_point+=ret_size;
				current_point += ret_size;
				buffer+=ret_size;
				write_size -= ret_size;
				_DEBUG("current point=%ld\n", file.current_point);
			}
		}
	}	
	return ans;
}

int CBB::_get_IOnode_socket(int IOnode_id, const std::string& ip)
{
	try
	{
		return IOnode_fd_map.at(IOnode_id);
	}
	catch(std::out_of_range& e)
	{
		struct sockaddr_in IOnode_addr;
		set_server_addr(ip, IOnode_addr);
		int IOnode_socket=Client::_connect_to_server(_client_addr, IOnode_addr);
		int ret;
		Send_flush(IOnode_socket, NEW_CLIENT);
		Recv(IOnode_socket, ret);
		if(SUCCESS == ret)
		{
			IOnode_fd_map.insert(std::make_pair(IOnode_id, IOnode_socket));
			return IOnode_socket;
		}
		else
		{
			close(IOnode_socket);
			return -1;
		}
	}
}

ssize_t CBB::_read(int fd, void *buffer, size_t size)
{
	int ret=0;
	CHECK_INIT();
	int fid=_fd_to_fid(fd);
	int master_socket=_get_master_socket_from_fd(fid);
	if(0 == size)
	{
		return size;
	}
	try
	{
		opened_file_info& file=_file_list.at(fid);
		ssize_t file_no=file.file_meta_p->file_no;
		_block_list_t blocks;
		_node_pool_t node_pool;
		Send(master_socket, READ_FILE);
		Send_flush(master_socket, file_no);
		Recv(master_socket, ret);
		if(SUCCESS == ret)
		{
			if(size+file.current_point > (size_t)file.file_meta_p->file_stat.st_size)
			{
				if(file.file_meta_p->file_stat.st_size>file.current_point)
				{
					size=file.file_meta_p->file_stat.st_size-file.current_point;
				}
				else
				{
					size=0;
				}
			}
			_get_blocks_from_master(master_socket, file.current_point, size, blocks, node_pool);
			return _read_from_IOnode(file, blocks, node_pool, static_cast<char *>(buffer), size);
		}
		else
		{
			errno=ret;
			return -1;
		}

	}
	catch(std::out_of_range &e)
	{
		errno = EBADFD;
		return -1;
	}
}

ssize_t CBB::_write(int fd, const void *buffer, size_t size)
{
	int ret=0;
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	int master_socket=_get_master_socket_from_fd(fid);
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
		_touch(fd);

		_block_list_t blocks;
		_node_pool_t node_pool;

		Send(master_socket, WRITE_FILE);
		Send_flush(master_socket, file_no);
		Recv(master_socket, ret);
		if(SUCCESS == ret)
		{
			Send_attr(master_socket, &file.file_meta_p->file_stat);
			_get_blocks_from_master(master_socket, current_point, size, blocks, node_pool);
			return _write_to_IOnode(file, blocks, node_pool,  static_cast<const char *>(buffer), size);
		}
		else
		{
			errno=ret;
			return -1;
		}
	}
	catch(std::out_of_range &e)
	{
		errno = EBADFD;
		return -1;
	}

}

int CBB::_write_update_file_size(opened_file_info& file, size_t size)
{
	if(file.current_point + size > (size_t)file.file_meta_p->file_stat.st_size)
	{
		file.file_meta_p->file_stat.st_size=file.current_point + size;
		return SUCCESS;
	}
	return FAILURE;
}

int CBB::_update_file_size(int fd, size_t size)
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

int CBB::_touch(int fd)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	try
	{
		opened_file_info& file=_file_list.at(fid);
		time_t new_time=time(NULL);
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

int CBB::_close(int fd)
{
	int ret=0;
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	int master_socket=_get_master_socket_from_fd(fid);
	try
	{
		opened_file_info& file=_file_list.at(fid);
		Send(master_socket, CLOSE_FILE);
		Send_flush(master_socket, file.file_meta_p->file_no);
		Recv(master_socket, ret);
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
			errno=ret;
			return -1;
		}
	}
	catch(std::out_of_range &e)
	{
		errno=EBADFD;
		return -1;
	}
}

int CBB::_flush(int fd)
{
	int ret=0;
	int fid=_fd_to_fid(fd);
	int master_socket=_get_master_socket_from_fd(fid);
	CHECK_INIT();
	try
	{
		opened_file_info& file=_file_list.at(fid);
		Send(master_socket, FLUSH_FILE);
		Send_flush(master_socket, file.file_meta_p->file_no);
		Recv(master_socket, ret);
		if(SUCCESS == ret)
		{
			return 0;
		}
		else
		{
			errno=ret;
			return -1;
		}
	}
	catch(std::out_of_range& e)
	{
		errno=EBADFD;
		return -1;
	}
}

off64_t CBB::_lseek(int fd, off64_t offset, int whence)
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

int CBB::_getattr(const char* path, struct stat* fstat)
{
	int ret=0;
	CHECK_INIT();
	int master_socket=_get_master_socket_from_path(path);
	if(SUCCESS == _get_local_attr(path, fstat))
	{
		_DEBUG("use local stat\n");
		return 0;
	}
	else
	{
		_DEBUG("connect to master\n");
		Send(master_socket, GET_ATTR); 
		Sendv_flush(master_socket, path, strlen(path));
		Recv(master_socket, ret);
		if(SUCCESS == ret)
		{
			_DEBUG("SUCCESS\n");
			Recv_attr(master_socket, fstat);
			//file_meta* file_meta_p=new file_meta(-1, path, 0, fstat);
			//++file_meta_p->open_count;
			//_path_file_meta_map.insert(std::make_pair(std::string(path), file_meta_p));
			return 0;
		}
		else
		{
			errno=ret;
			return -errno;
		}
	}
}

int CBB::_readdir(const char * path, dir_t& dir)const
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int master_socket=_get_master_socket_from_path(path);
	Send(master_socket, READ_DIR); 
	Sendv_flush(master_socket, path, strlen(path));
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		_DEBUG("success\n");
		int count=0;
		char *path=NULL;
		Recv(master_socket, count);
		for(int i=0; i<count; ++i)
		{
			Recvv(master_socket, &path);
			dir.push_back(std::string(path));
			delete[] path;
		}
		Recv(master_socket, count);
		return 0;
	}
	else
	{
		errno=ret;
		return -errno;
	}
}

int CBB::_rmdir(const char * path)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int master_socket=_get_master_socket_from_path(path);
	Send(master_socket, RM_DIR); 
	Sendv_flush(master_socket, path, strlen(path));
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		errno=ret;
		return -errno;
	}
}

int CBB::_unlink(const char * path)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	_close_local_opened_file(path);
	int master_socket=_get_master_socket_from_path(path);
	Send(master_socket, UNLINK); 
	Sendv_flush(master_socket, path, strlen(path));
	Recv(master_socket, ret);

	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		errno=ret;
		return -errno;
	}
}

int CBB::_close_local_opened_file(const char* path)
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

int CBB::_access(const char* path, int mode)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int master_socket=_get_master_socket_from_path(path);
	Send(master_socket, ACCESS); 
	Sendv(master_socket, path, strlen(path));
	Send_flush(master_socket, mode);
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		errno=ret;
		return -errno;
	}
}

int CBB::_stat(const char* path, struct stat* buf)
{
	CHECK_INIT();
	int ret;
	memset(buf, 0, sizeof(struct stat));
	int master_socket=_get_master_socket_from_path(path);
	Send(master_socket, GET_FILE_META);
	Sendv_flush(master_socket, path, strlen(path));
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		Recvv_pre_alloc(master_socket, buf, sizeof(struct stat));
		return 0;
	}
	else
	{
		return -1;
	}
}

int CBB::_rename(const char* old_name, const char* new_name)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int master_socket=_get_master_socket_from_path(old_name);

	_path_file_meta_map_t::iterator it=_path_file_meta_map.find(old_name);
	if(_path_file_meta_map.end() != it)
	{
		
		_path_file_meta_map_t::iterator new_it=_path_file_meta_map.insert(std::make_pair(new_name, it->second)).first;
		_path_file_meta_map.erase(it);
		new_it->second->it=new_it;
	}
	Send(master_socket, RENAME); 
	Sendv(master_socket, old_name, strlen(old_name));
	Sendv_flush(master_socket, new_name, strlen(new_name));
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		errno=ret;
		return -errno;
	}
}

int CBB::_mkdir(const char* path, mode_t mode)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int master_socket=_get_master_socket_from_path(path);
	Send(master_socket, MKDIR); 
	Sendv(master_socket, path, strlen(path));
	Send_flush(master_socket, mode);
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		errno=ret;
		return -errno;
	}
}

off64_t CBB::_tell(int fd)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	return _file_list.at(fid).current_point;
}

bool CBB::_interpret_path(const char *path)
{
	if(NULL == mount_point)
	{
		return false;
	}
	if(!strncmp(path, mount_point, strlen(mount_point)))
	{
		return true;
	}
	return false;
}

bool CBB::_interpret_fd(int fd)
{
	if(fd<INIT_FD)
	{
		return false;
	}
	return true;
}

void CBB::_format_path(const char *path, std::string &formatted_path)
{
	char *real_path=static_cast<char *>(malloc(PATH_MAX));
	realpath(path, real_path);

	formatted_path=std::string(real_path);
	free(real_path);
	return;
}

void CBB::_get_relative_path(const std::string &formatted_path, std::string &relative_path)
{
	const char *pointer=formatted_path.c_str()+strlen(mount_point);
	relative_path=std::string(pointer);
	return;
}

inline int CBB::_fd_to_fid(int fd)
{
	return fd-INIT_FD;
}

inline int CBB::_fid_to_fd(int fid)
{
	return fid+INIT_FD;
}

size_t CBB::_get_file_size(int fd)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	return _file_list.at(fid).file_meta_p->file_stat.st_size;
}

int CBB::_get_local_attr(const char* path, struct stat *file_stat)
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

int CBB::_truncate(const char* path, off64_t size)
{
	int ret=0;
	_DEBUG("connect to master\n");
	CHECK_INIT();
	int master_socket=_get_master_socket_from_path(path);
	Send(master_socket, TRUNCATE); 
	Sendv(master_socket, path, strlen(path));
	Send_flush(master_socket, size);
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		return 0;
	}
	else
	{
		errno=ret;
		return -errno;
	}
}

int CBB::_ftruncate(int fd, off64_t size)
{
	int fid=_fd_to_fid(fd);
	CHECK_INIT();
	opened_file_info& file=_file_list.at(fid);
	file.file_meta_p->file_stat.st_size=size;
	_touch(fd);
	return _truncate(file.file_meta_p->it->first.c_str(), size);
}

int CBB::_get_master_socket_from_path(const std::string& path)const
{
	static std::hash<std::string> string_hash;
	static size_t size=master_socket_list.size();
	return master_socket_list.at(string_hash(path)%size);
}

inline int CBB::_get_master_socket_from_fd(int fd)const
{
	return _file_list.at(fd).file_meta_p->master_socket;
}
