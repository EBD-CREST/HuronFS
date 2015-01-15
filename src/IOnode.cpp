/*
 * IOnode.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#include "include/IOnode.h"
#include "include/IO_const.h"
#include "include/Communication.h"

const char *IOnode::IONODE_MOUNT_POINT="CBB_IONODE_MOUNT_POINT";

IOnode::block::block(off64_t start_point, size_t size, bool dirty_flag, bool valid) throw(std::bad_alloc):
	size(size),
	data(NULL),
	start_point(start_point),
	dirty_flag(dirty_flag),
	valid(INVALID)
{
	data=malloc(size);
	if( NULL == data)
	{
	   throw std::bad_alloc();
	}
	return;
}

IOnode::block::~block()
{
	free(data);
}

IOnode::block::block(const block & src):
	size(src.size),
	data(src.data), 
	start_point(src.start_point),
	dirty_flag(src.dirty_flag),
	valid(src.valid)
{};

IOnode::IOnode(const std::string& master_ip,  int master_port) throw(std::runtime_error):
	Server(IONODE_PORT), 
	Client(), 
	_node_id(-1),
	_files(file_blocks_t()),
	_file_path(file_path_t()),
	_current_block_number(0), 
	_MAX_BLOCK_NUMBER(MAX_BLOCK_NUMBER), 
	_memory(MEMORY), 
	_master_port(MASTER_PORT),
	_mount_point(std::string())
{
	memset(&_master_conn_addr, 0, sizeof(_master_conn_addr));
	memset(&_master_addr, 0, sizeof(_master_addr));
	if(-1  ==  (_node_id=_regist(master_ip,  master_port)))
	{
		throw std::runtime_error("Get Node Id Error"); 
	}
	const char *IOnode_mount_point=getenv(IONODE_MOUNT_POINT);
	if( NULL == IOnode_mount_point)
	{
		throw std::runtime_error("please set IONODE_MOUNT_POINT environment value");
	}
	_mount_point=std::string(IOnode_mount_point);
	try
	{
		Server::_init_server();
	}
	catch(std::runtime_error& e)
	{
		//_unregist();
		throw;
	}
}

int IOnode::_regist(const std::string& master_ip, int master_port) throw(std::runtime_error)
{ 
	_master_conn_addr.sin_family = AF_INET;
	_master_conn_addr.sin_addr.s_addr = htons(INADDR_ANY);
	_master_conn_addr.sin_port = htons(MASTER_CONN_PORT);

	_master_addr.sin_family = AF_INET;
	_master_addr.sin_port = htons(_master_port);
	if( 0  ==  inet_aton(master_ip.c_str(), &_master_addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error");
	}
	int master_socket;
	try
	{
		master_socket=Client::_connect_to_server(_master_conn_addr, _master_addr); 
	}
	catch(std::runtime_error &e)
	{
		throw;
	}
	Send(master_socket, REGIST);
	Send(master_socket, _memory);
	int id=-1;
	Recv(master_socket, id);
	Server::_add_socket(master_socket);
	int open;
	Recv(master_socket, open);
	return id; 
}

int IOnode::_parse_new_request(int sockfd, const struct sockaddr_in& client_addr)
{
	int request, ans=SUCCESS; 
	Recv(sockfd, request); 
	switch(request)
	{
/*	case SERVER_SHUT_DOWN:
		ans=SERVER_SHUT_DOWN; break; */
	case READ_FILE:
		_send_data(sockfd);break;
	case WRITE_FILE:
		_receive_data(sockfd);break;
/*	dafault:
		break; */
	}
	return ans; 
}

//request from master
int IOnode::_parse_registed_request(int sockfd)
{
	int request, ans=SUCCESS; 
	Recv(sockfd, request); 
	switch(request)
	{
	case OPEN_FILE:
		_open_file(sockfd);break;
	case READ_FILE:
		_read_file(sockfd);break;
	case WRITE_FILE:
		_write_file(sockfd);break;
/*	case I_AM_SHUT_DOWN:
		ans=SERVER_SHUT_DOWN;break;*/
	case FLUSH_FILE:
		_write_back_file(sockfd);break;
	case CLOSE_FILE:
		_close_file(sockfd);break;
/*	default:
		break; */
	}
	return ans; 
}

int IOnode::_send_data(int sockfd)
{
	ssize_t file_no;
	off64_t start_point;
	off64_t offset;
	size_t size;
	Recv(sockfd, file_no);
	Recv(sockfd, start_point);
	Recv(sockfd, offset);
	Recv(sockfd, size);
	try
	{
		const std::string& path = _file_path[file_no];
		block* requested_block=_files.at(file_no).at(start_point);
		if(INVALID == requested_block->valid)
		{
			_read_from_storage(path, requested_block);
			requested_block->valid = VALID;
		}
		Send(sockfd, SUCCESS);
		Sendv_pre_alloc(sockfd, reinterpret_cast<char*>(requested_block->data)+offset, size);
		close(sockfd);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		Send(sockfd, FILE_NOT_FOUND);
		close(sockfd);
		return FAILURE;
	}
}

int IOnode::_open_file(int sockfd)
{
	ssize_t file_no; 
	int flag=0;
	char *path_buffer=NULL; 
	Recv(sockfd, file_no);
	Recv(sockfd, flag);
	Recvv(sockfd, &path_buffer); 
	size_t start_point, block_size;
	block_info_t *blocks=NULL; 
	Recv(sockfd, start_point); 
	Recv(sockfd, block_size); 
	if(_files.end() != _files.find(file_no))
	{
		blocks=&(_files.at(file_no)); 
	}
	else
	{
		blocks=&(_files[file_no]); 
	}
	blocks->insert(std::make_pair(start_point, new block(start_point, block_size, CLEAN, INVALID)));
	if(_file_path.end() == _file_path.find(file_no))
	{
		_file_path.insert(std::make_pair(file_no, std::string(path_buffer)));
	}
	delete path_buffer; 
	return SUCCESS; 
}

int IOnode::_read_file(int sockfd)
{
	off64_t start_point=0;
	size_t size=0;
	ssize_t file_no=0;
	Recv(sockfd, file_no);
	Recv(sockfd, start_point);
	Recv(sockfd, size);
	const std::string& path=_file_path.at(file_no);
	block_info_t &file=_files.at(file_no);
	for(block_info_t::iterator it=file.find(start_point);
			it != file.end();++it)
	{
		_read_from_storage(path, it->second);
	}
	return SUCCESS;
}

size_t IOnode::_read_from_storage(const std::string& path, block* block_data)throw(std::runtime_error)
{
	off64_t start_point=block_data->start_point;
	std::string true_path(_mount_point+path);
	int fd=open64(true_path.c_str(), O_RDONLY); 
	if(-1 == fd)
	{
		perror("File Open Error"); 
	}
	if(-1  == lseek(fd, start_point, SEEK_SET))
	{
		perror("Seek"); 
	}
	ssize_t ret; 
	struct iovec vec;
	char *buffer=reinterpret_cast<char*>(block_data->data);
	size_t size=block_data->size;
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
	return block_data->size-size;
}

IOnode::block* IOnode::_buffer_block(off64_t start_point, size_t size)throw(std::runtime_error)
{
	try
	{
		block *new_block=new block(start_point, size, DIRTY, VALID); 
		return new_block; 
	}
	catch(std::bad_alloc &e)
	{
		throw std::runtime_error("Memory Alloc Error"); 
	}
}

int IOnode::_write_file(int clientfd)
{
	ssize_t file_no;
	off64_t start_point;
	size_t size;
	char *file_path=NULL;
	Recv(clientfd, file_no);
	Recvv(clientfd, &file_path);
	Recv(clientfd, start_point);
	Recv(clientfd, size);
	try
	{
		block_info_t &blocks=_files.insert(std::make_pair(file_no, block_info_t())).first->second;
		blocks.insert(std::make_pair(start_point, new block(start_point, size, DIRTY, VALID)));
		if(_file_path.end() == _file_path.find(file_no))
		{
			_file_path.insert(std::make_pair(file_no, std::string(file_path)));
		}
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		return FILE_NOT_FOUND;
	}
}

int IOnode::_receive_data(int clientfd)
{
	ssize_t file_no;
	off64_t start_point;
	off64_t offset;
	size_t size;
	Recv(clientfd, file_no);
	Recv(clientfd, start_point);
	Recv(clientfd, offset);
	Recv(clientfd, size);
	try
	{
		block_info_t &blocks=_files.at(file_no);
		block* _block=blocks.at(start_point);
		Send(clientfd, SUCCESS);
		Recvv_pre_alloc(clientfd, reinterpret_cast<char*>(_block->data)+offset, size);
		_block->dirty_flag=DIRTY;
		close(clientfd);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		Send(clientfd, FILE_NOT_FOUND);
		close(clientfd);
		return FAILURE;
	}
}

int IOnode::_write_back_file(int clientfd)
{
	ssize_t file_no;
	off64_t start_point;
	size_t size;
	Recv(clientfd, file_no);
	Recv(clientfd, start_point);
	Recv(clientfd, size);
	try
	{
		block_info_t &blocks=_files.at(file_no);
		block* _block=blocks.at(start_point);
		Send(clientfd, SUCCESS);
		const std::string &path=_file_path.at(file_no);
		_write_to_storage(path, _block);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		Send(clientfd, FILE_NOT_FOUND);
		return FAILURE;
	}
}

size_t IOnode::_write_to_storage(const std::string& path, const block* block_data)throw(std::runtime_error)
{

	/*FILE *fp = fopen(path.c_str(),"a");
	if( NULL == fp)
	{
		perror("Open File");
		throw std::runtime_error("Open File Error\n");
	}
	if(-1 == fseek(fp, block_data->start_point, SEEK_SET))
	{
		perror("Seek"); 
		throw std::runtime_error("Seek File Error"); 
	}
	fwrite(block_data->data, sizeof(char), block_data->size, fp);
	fclose(fp);*/
	std::string true_path(_mount_point+path);
	int fd = open64(true_path.c_str(),O_WRONLY|O_CREAT|O_SYNC);
	if( -1 == fd)
	{
		perror("Open File");
		throw std::runtime_error("Open File Error\n");
	}
	off64_t pos;
	if(-1 == (pos=lseek64(fd, block_data->start_point, SEEK_SET)))
	{
		perror("Seek"); 
		throw std::runtime_error("Seek File Error"); 
	}
	printf("seek %ld\n",pos);
	struct iovec iov;
	iov.iov_base=const_cast<void*>(block_data->data);
	iov.iov_len=block_data->size;
	writev(fd, &iov, 1);
	//puts((char*)block_data->data);
//	write(STDOUT_FILENO, (char*)block_data->data, length);
//	printf("%s, %lu\n", block_data->data, block_data->size);
	close(fd);
	return block_data->size;
}

int IOnode::_flush_file(int sockfd)
{
	ssize_t file_no;
	Recv(sockfd, file_no);
	std::string &path=_file_path.at(file_no);
	block_info_t &blocks=_files.at(file_no);
	for(block_info_t::iterator it=blocks.begin();
			it != blocks.end();++it)
	{
		block* _block=it->second;
		if(DIRTY == _block->dirty_flag)
		{
			_write_to_storage(path, _block);
			_block->dirty_flag=CLEAN;
		}
	}
	//Send(sockfd, SUCCESS);
	return SUCCESS;
}

int IOnode::_close_file(int sockfd)
{
	ssize_t file_no;
	Recv(sockfd, file_no);
	std::string &path=_file_path.at(file_no);
	block_info_t &blocks=_files.at(file_no);
	for(block_info_t::iterator it=blocks.begin();
			it != blocks.end();++it)
	{
		block* _block=it->second;
		if(DIRTY == _block->dirty_flag)
		{
			_write_to_storage(path, _block);
			puts((char*)_block->data);
		}
		delete _block;
	}
	_files.erase(file_no);
	_file_path.erase(file_no);
	//Send(sockfd, SUCCESS);
	return SUCCESS;
}
