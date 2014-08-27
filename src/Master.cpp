/*OB`
 * Master.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */
#include <sstream>
#include <cstring>

#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "include/Master.h"
#include "include/IO_const.h"
#include "include/Communication.h"

Master::file_info::file_info(const std::string& path, size_t size, size_t block_size, const node_t& IOnodes, int flag):
	path(path), 
	p_node(IOnodes), 
	nodes(node_pool_t()),
	size(size),
	block_size(block_size),
	flag(flag)
{
	for(node_t::const_iterator it=p_node.begin();
			it != p_node.end();++it)
	{
		nodes.insert(it->second);
	}
	return;
}

Master::node_info::node_info(ssize_t id, const std::string& ip, size_t total_memory, int socket):
	node_id(id),
	ip(ip), 
	stored_files(file_t()), 
	avaliable_memory(total_memory), 
	total_memory(total_memory), 
	socket(socket)
{}

Master::Master()throw(std::runtime_error):
	Server(MASTER_PORT), 
	_registed_IOnodes(IOnode_t()), 
	_file_no(file_no_t()), 
	_buffered_files(File_t()), 
	_IOnode_socket(IOnode_sock_t()), 
	_node_number(0), 
	_file_number(0), 
	_node_id_pool(new bool[MAX_NODE_NUMBER]), 
	_file_no_pool(new bool[MAX_FILE_NUMBER]), 
	_now_node_number(0), 
	_now_file_no(0) 
{
	memset(_node_id_pool, 0, MAX_NODE_NUMBER*sizeof(bool)); 
	memset(_file_no_pool, 0, MAX_FILE_NUMBER*sizeof(bool)); 
	try
	{
		_init_server(); 
	}
	catch(std::runtime_error)
	{
		throw; 
	}
}

Master::~Master()
{
	Server::stop_server(); 
	delete _node_id_pool;
	delete _file_no_pool; 
}

ssize_t Master::_add_IO_node(const std::string& node_ip, std::size_t total_memory, int socket)
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
	_registed_IOnodes.insert(std::make_pair(id, node_info(id, node_ip, total_memory, socket)));
	node_info *node=&(_registed_IOnodes.at(id)); 
	_IOnode_socket.insert(std::make_pair(socket, node)); 
	Server::_add_socket(socket); 
	++_node_number; 
	//int open;
	//Recv(socket, open);
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
	return id;
}


ssize_t Master::_get_node_id()
{
	if(MAX_NODE_NUMBER == _node_number)
	{
		return -1;
	}
	for(; _now_node_number<MAX_NODE_NUMBER; ++_now_node_number)
	{
		if(!_node_id_pool[_now_node_number])
		{
			_node_id_pool[_now_node_number]=true; 
			return _now_node_number; 
		}
	}
	for(_now_node_number=0;  _now_node_number<MAX_NODE_NUMBER; ++_now_node_number)
	{
		if(!_node_id_pool[_now_node_number])
		{
			_node_id_pool[_now_node_number]=true; 
			return _now_node_number;
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
	for(; _now_file_no<MAX_FILE_NUMBER; ++_now_file_no)
	{
		if(!_file_no_pool[_now_file_no])
		{
			_file_no_pool[_now_file_no]=true; 
			return _now_file_no; 
		}
	}
	for(_now_file_no=0;  _now_file_no<MAX_FILE_NUMBER; ++_now_file_no)
	{
		if(!_file_no_pool[_now_file_no])
		{
			_file_no_pool[_now_file_no]=true; 
			return _now_file_no;
		}
	}
	return -1;
}

void Master::_send_node_info(int clientfd, std::string& ip)const 
{
	fprintf(stderr, "requery for IOnode info, ip=%s\n", ip.c_str()); 
	Send(clientfd, static_cast<int>(_registed_IOnodes.size()));

	for(IOnode_t::const_iterator it=_registed_IOnodes.begin(); it!=_registed_IOnodes.end(); ++it)
	{
		const node_info &node=it->second;
		Sendv(clientfd, node.ip.c_str(), node.ip.size());
		Send(clientfd, node.total_memory);
		Send(clientfd, node.avaliable_memory);
	}
	close(clientfd);
	return; 
}

Master::IOnode_t::iterator Master::_find_by_ip(const std::string &ip)
{
	IOnode_t::iterator it=_registed_IOnodes.begin();
	for(; it != _registed_IOnodes.end() && ! (it->second.ip == ip); ++it);
	return it;
}

int Master::_parse_new_request(int clientfd, const struct sockaddr_in& client_addr)
{
	int request, ans=SUCCESS;
	std::string ip(inet_ntoa(client_addr.sin_addr));
	Recv(clientfd, request);
	switch(request)
	{
	case REGIST:
		_parse_regist_IOnode(clientfd, ip);break;
	case PRINT_NODE_INFO:
		_send_node_info(clientfd, ip);break;
	case GET_FILE_INFO:
		_send_file_info(clientfd, ip);break; 
	case OPEN_FILE:
		_parse_open_file(clientfd, ip);break; 
	case READ_FILE:
		_parse_read_file(clientfd, ip);break;
	case WRITE_FILE:
		_parse_write_file(clientfd, ip);break;
	case FLUSH_FILE:
		_parse_flush_file(clientfd, ip);break;
	case CLOSE_FILE:
		_parse_close_file(clientfd, ip);break;
	case GET_FILE_META:
		_send_file_meta(clientfd, ip);break;
/*	default:
		Send(clientfd, UNRECOGNISTED); 
		close(clientfd); */
	}
	return ans;
}

int Master::_parse_regist_IOnode(int clientfd,const std::string& ip)
{
	size_t total_memory;
	fprintf(stderr, "regist IOnode\nip=%s\n",ip.c_str());
	Recv(clientfd, total_memory);
	Send(clientfd, _add_IO_node(ip, total_memory, clientfd));
	return 1;
}

void Master::_send_file_meta(int clientfd, std::string& ip)const
{
	ssize_t file_no; 
	fprintf(stderr, "requery for File meta data, ip=%s\n", ip.c_str());
	Recv(clientfd, file_no); 
	const file_info *file=NULL; 
	try
	{
		file=&(_buffered_files.at(file_no));
		Send(clientfd, SUCCESS);
	}
	catch(std::out_of_range &e)
	{
		const char OUT_OF_RANGE[]="out of range\n"; 
		Send(clientfd, OUT_OF_RANGE);
		return; 
	}
	Send(clientfd, file->size);
	Send(clientfd, file->block_size);
	close(clientfd);
	return ;
}

void Master::_send_file_info(int clientfd, std::string& ip)const
{
	ssize_t file_no; 
	fprintf(stderr, "requery for File info, ip=%s\n", ip.c_str());
	Recv(clientfd, file_no); 
	const file_info *file=NULL; 
	try
	{
		file=&(_buffered_files.at(file_no));
		Send(clientfd, SUCCESS);
	}
	catch(std::out_of_range &e)
	{
		const char OUT_OF_RANGE[]="out of range\n"; 
		Send(clientfd, OUT_OF_RANGE);
		return; 
	}
	Send(clientfd, file->size);
	Send(clientfd, file->block_size);
	
	Send(clientfd, static_cast<int>(file->p_node.size()));

	for(node_t::const_iterator it=file->p_node.begin(); it!=file->p_node.end(); ++it)
	{
		Send(clientfd, it->first);
		const std::string& ip=_registed_IOnodes.at(it->second).ip; 
		Sendv(clientfd, ip.c_str(), ip.size());
	}
	close(clientfd);
	return; 
}	

int Master::_parse_registed_request(int clientfd)
{
	int request, ans=SUCCESS; 
	ssize_t id;
	Recv(clientfd, request); 
	switch(request)
	{
	case I_AM_SHUT_DOWN:
		id=_IOnode_socket.at(clientfd)->node_id;
		fprintf(stderr, "IOnode %ld shutdown\nIP Address=%s, Unregisted\n", id, _registed_IOnodes.at(id).ip.c_str()), 
		_delete_IO_node(clientfd);break;
/*	default:
		Send(clientfd, UNRECOGNISTED); */
	}
	return ans; 
}

int Master::_parse_open_file(int clientfd, std::string& ip)
{
	char *file_path;
	fprintf(stderr, "request for open file, ip=%s\n", ip.c_str()); 
	Recvv(clientfd, &file_path); 
	int flag ,ret=SUCCESS;
	Recv(clientfd, flag); 
	try
	{
		ssize_t file_no;
		_open_file(file_path, flag, file_no); 
		size_t size=_buffered_files.at(file_no).size;
		delete file_path; 
		Send(clientfd, SUCCESS);
		Send(clientfd, file_no);
		Send(clientfd, size);
		/*Send(clientfd, static_cast<int>(nodes.size()));
		for(node_t::const_iterator it=nodes.begin(); it!=nodes.end(); ++it)
		{
			Send(clientfd, it->first);
			std::string ip=_registed_IOnodes.at(it->second).ip;
			Sendv(clientfd, ip.c_str(),ip.size());
		}*/
	}
	catch(std::runtime_error &e)
	{
		fprintf(stderr, "%s\n",e.what());
		Send(clientfd, UNKNOWN_ERROR);
		ret=FAILURE;
	}
	catch(std::invalid_argument &e)
	{
		fprintf(stderr, "%s\n",e.what());
		Send(clientfd, FILE_NOT_FOUND);
		ret=FAILURE;
	}
	catch(std::bad_alloc &e)
	{
		Send(clientfd, TOO_MANY_FILES);
		ret=FAILURE;
	}
	close(clientfd);
	return ret;
}

const Master::node_t& Master::_open_file(const char* file_path, int flag, ssize_t& new_file_no)throw(std::runtime_error, std::invalid_argument, std::bad_alloc)
{
	//open in read
	file_no_t::iterator it; 
	//file not buffered
	const file_info *file=NULL; 
	if(_file_no.end() ==  (it=_file_no.find(file_path)))
	{
		new_file_no=_get_file_no(); 
		if(-1 != new_file_no)
		{
			//open file
			if(flag == O_RDONLY || flag&O_RDWR)
			{
				size_t file_length=0,block_size=0;
				node_t nodes=_send_request_to_IOnodes(file_path, new_file_no, flag, file_length, block_size);
				file_info new_file(file_path, file_length,block_size,nodes, flag); 
				_buffered_files.insert(std::make_pair(new_file_no, new_file));
				file=&(_buffered_files.at(new_file_no)); 
				_file_no.insert(std::make_pair(file_path, new_file_no)); 
				for(node_t::const_iterator it=nodes.begin(); 
						nodes.end()!=it; ++it)
				{
					//send read request to each IOnode
					//buffer requset
					//file_no
					//file_path
					//start_point
					//block_size
					int socket=_registed_IOnodes.at(it->second).socket;
					Send(socket, READ_FILE);
					Send(socket, new_file_no); 
					Send(socket, it->first); 
					Send(socket, block_size); 
				}
			}
			else
			{
				size_t file_length=0,block_size=0;
				file_info new_file(file_path, file_length, block_size, node_t(), flag); 
				_buffered_files.insert(std::make_pair(new_file_no, new_file));
				file=&(_buffered_files.at(new_file_no)); 
				_file_no.insert(std::make_pair(file_path, new_file_no)); 
			}
		}
		else
		{
			throw std::bad_alloc();
		}
	}
	//file buffered
	else
	{
		file=&(_buffered_files.at(it->second)); 
	}
	return file->p_node;
}

Master::node_t Master::_send_request_to_IOnodes(const char *file_path, ssize_t file_no, int flag, size_t& file_length,size_t& block_size)throw(std::invalid_argument)
{
	int fd=open(file_path, flag);
	
	if(-1 == fd)
	{
		throw std::invalid_argument("file can not open");
	}

	struct stat file_stat;
	fstat(fd, &file_stat);
	file_length=file_stat.st_size;
	block_size=_get_block_size(file_length); 
	close(fd);
	// send node number
	node_t nodes=_select_IOnode(file_length, block_size);
	for(node_t::const_iterator it=nodes.begin(); 
			nodes.end()!=it; ++it)
	{
		//send read request to each IOnode
		//buffer requset
		//file_no
		//file_path
		//start_point
		//block_size
		int socket=_registed_IOnodes.at(it->second).socket;
		Send(socket, OPEN_FILE);
		Send(socket, file_no); 
		Send(socket, flag);
		Sendv(socket, file_path, strlen(file_path));
		Send(socket, it->first); 
		Send(socket, block_size); 
	}
	return nodes; 
}

Master::node_t Master::_select_IOnode(size_t file_size, size_t block_size)const
{
	int node_number = (file_size+block_size-1)/block_size,count=(node_number+_node_number-1)/_node_number, count_node=0;
	node_t nodes; 
	off_t start_point=0;
	for(IOnode_t::const_iterator it=_registed_IOnodes.begin();
			_registed_IOnodes.end() != it;++it)
	{
		for(int i=0;i<count && (count_node++)<node_number;++i)
		{
			nodes.insert(std::make_pair(start_point, it->first));
			start_point += block_size;
		}
	}
	return nodes; 
}

int Master::_parse_read_file(int clientfd, std::string& ip)
{
	fprintf(stderr, "request for reading ip=%s\n",ip.c_str());
	_send_file_info(clientfd, ip);
	close(clientfd);
	return 1;
}

int Master::_parse_write_file(int clientfd, std::string& ip)
{
	fprintf(stderr, "request for writing ip=%s\n",ip.c_str());
	ssize_t file_no;
	size_t size, block_size;
	off_t start_point;
	Recv(clientfd, file_no);
	Recv(clientfd, start_point);
	Recv(clientfd, size);
	int fd=open(_buffered_files.at(file_no).path.c_str(), O_CREAT, S_IRUSR|S_IWUSR);
	if(-1 == fd)
	{
		fprintf(stderr, "file create error\n");
		return FAILURE;
	}
	close(fd);
	try
	{
		file_info &file=_buffered_files.at(file_no);
		block_size=_get_block_size(size);
		file.block_size=block_size;
		if(0 == file.size)
		{
			file.size=size;
		}
		node_t nodes=_select_IOnode(size, block_size);
		for(node_t::const_iterator it=nodes.begin(); 
				nodes.end()!=it; ++it)
		{
			//send read request to each IOnode
			//buffer requset
			//file_no
			//file_path
			//start_point
			//block_size
			int socket=_registed_IOnodes.at(it->second).socket;
			size_t my_block_size=it->first+block_size > file.size?file.size-it->first:block_size;
			Send(socket, WRITE_FILE);
			Send(socket, file_no);
			Sendv(socket, file.path.c_str(), file.path.size());
			Send(socket, it->first); 
			Send(socket, my_block_size); 
		}
		file.p_node=nodes;
		for(node_t::const_iterator it=file.p_node.begin();
				it !=file.p_node.end();++it)
		{
			file.nodes.insert(it->second);
		}
		Send(clientfd, SUCCESS);	
		Send(clientfd, static_cast<int>(file.p_node.size()));
		Send(clientfd, file.size);
		Send(clientfd, file.block_size);
		for(node_t::const_iterator it=file.p_node.begin(); it!=file.p_node.end(); ++it)
		{
			Send(clientfd, it->first);
			const std::string& ip=_registed_IOnodes.at(it->second).ip; 
			Sendv(clientfd, ip.c_str(), ip.size());
		}
		close(clientfd);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		Send(clientfd, FAILURE);	
		close(clientfd);
		return FAILURE;
	}
}

size_t Master::_get_block_size(size_t size)
{
	return (size+_node_number-1)/_node_number;
}

int Master::_parse_flush_file(int clientfd, std::string& ip)
{
	fprintf(stderr, "request for writing ip=%s\n",ip.c_str());
	ssize_t file_no;
	Recv(clientfd, file_no);
	file_info &file=_buffered_files.at(file_no);
	Send(clientfd, SUCCESS);
	try
	{
		for(node_pool_t::iterator it=file.nodes.begin();
				it != file.nodes.end();++it)
		{
			int socket=_IOnode_socket.at(*it)->socket;
			Send(socket, FLUSH_FILE);
			Send(socket, file_no);
//			int ret=0;
//			Recv(socket, ret);
		}
		close(clientfd);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		Send(clientfd, FAILURE);
		close(clientfd);
		return FAILURE;
	}
}

int Master::_parse_close_file(int clientfd, std::string& ip)
{
	fprintf(stderr, "request for writing ip=%s\n",ip.c_str());
	ssize_t file_no;
	Recv(clientfd, file_no);
	file_info &file=_buffered_files.at(file_no);
	Send(clientfd, SUCCESS);
	close(clientfd);
	try
	{
		for(node_pool_t::iterator it=file.nodes.begin();
				it != file.nodes.end();++it)
		{
			int socket=_registed_IOnodes.at(*it).socket;
			Send(socket, CLOSE_FILE);
			Send(socket, file_no);
//			int ret=0;
//			Recv(socket, ret);
		}
		File_t::iterator file=_buffered_files.find(file_no);
		std::string &path=file->second.path;
		_file_no.erase(path);
		_buffered_files.erase(file);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		Send(clientfd, FAILURE);
		close(clientfd);
		return FAILURE;
	}
}

