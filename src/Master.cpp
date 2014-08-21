/*OB
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

#include "include/Master.h"
#include "include/IO_const.h"
#include "include/Communication.h"

Master::file_info::file_info(const std::string& path, std::size_t size, const node_t& IOnodes, int flag):
	path(path), 
	IOnodes(IOnodes), 
	size(size), 
	flag(flag)
{}

Master::node_info::node_info(const std::string& ip, std::size_t total_memory, int socket):
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
	_registed_IOnodes.insert(std::make_pair(id, node_info(node_ip, total_memory, socket)));
	node_info *node=&(_registed_IOnodes.at(id)); 
	_IOnode_socket.insert(std::make_pair(socket, node)); 
	Server::_add_socket(socket); 
	++_node_number; 
	return id; 
}

ssize_t Master::_delete_IO_node(const std::string& node_ip)
{
	IOnode_t::iterator it=_find_by_ip(node_ip);
	if(it != _registed_IOnodes.end())
	{
		ssize_t id=it->first;
		int socket=it->second.socket; 
		_registed_IOnodes.erase(it);
		Server::_delete_socket(socket); 
		close(socket); 
		_IOnode_socket.erase(socket); 
		_node_id_pool[id]=false;
		--_node_number; 
		return id;
	}
	else
	{
		return -1;
	}
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

void Master::_print_node_info(int clientfd)const 
{
	int count=0;
	std::ostringstream output; 
	for(IOnode_t::const_iterator it=_registed_IOnodes.begin(); it!=_registed_IOnodes.end(); ++it)
	{
		const node_info &node=it->second;
		output << "IOnode :" << ++count  << std::endl << "ip=" << node.ip << std::endl<< "total_memory=" << node.total_memory << std::endl << "avaliable_memory=" << node.avaliable_memory << std::endl;
	}
	std::string s=output.str(); 
	Sendv(clientfd, s.c_str(), s.size()); 
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
	int request,tmp, ans=SUCCESS;
	Recv(clientfd, request);
	switch(request)
	{
	case REGIST:
		fprintf(stderr, "regist IOnode\n");
		Recv(clientfd, tmp);
		Send(clientfd, _add_IO_node(std::string(inet_ntoa(client_addr.sin_addr)), tmp, clientfd));break;
	case UNREGIST:
		fprintf(stderr, "unregist IOnode\n");
		_delete_IO_node(std::string(inet_ntoa(client_addr.sin_addr)));break;
	case PRINT_NODE_INFO:
		fprintf(stderr, "requery for IOnode info\n"); 
		_print_node_info(clientfd);
		close(clientfd); break; 
	case VIEW_FILE_INFO:
		fprintf(stderr, "requery for File info\n");
		_print_file_info(clientfd); 
		close(clientfd); break; 
	case OPEN_FILE:
		fprintf(stderr, "requset for open file"); 
		_parse_open_file(clientfd); 
		close(clientfd); break; 
	default:
		Send(clientfd, UNRECOGNISTED); 
		close(clientfd); 
	}
	return ans;
}

void Master::_print_file_info(int clientfd)const
{
	ssize_t file_no; 
	Recv(clientfd, file_no); 
	std::ostringstream output; 
	const file_info *file=NULL; 
	try
	{
		file=&(_buffered_files.at(file_no)); 
	}
	catch(std::out_of_range &e)
	{
		const char OUT_OF_RANGE[]="out of range\n"; 
		Sendv(clientfd, OUT_OF_RANGE, sizeof(OUT_OF_RANGE));
		return; 
	}
	for(node_t::const_iterator it=file->IOnodes.begin(); it!=file->IOnodes.end(); ++it)
	{
		output << "Start point:" << it->first << std::endl << "IOnode number" << it->second << std::endl<< "ip=" << _registed_IOnodes.at(it->second).ip << std::endl;
	}
	std::string s=output.str(); 
	Sendv(clientfd, s.c_str(), s.size()); 
	return; 
}	

int Master::_parse_registed_request(int clientfd)
{
	int request, ans=SUCCESS; 
	Recv(clientfd, request); 
	switch(request)
	{
	default:
		Send(clientfd, UNRECOGNISTED); 
	}
	return ans; 
}

int Master::_parse_open_file(int clientfd)
{
	char *file_path;
	Recvv(clientfd, &file_path); 
	int flag; 
	Recv(clientfd, flag); 
	_open_file(file_path, flag); 
	delete file_path; 
	return 1; 
}

const Master::node_t& Master::_open_file(const char* file_path, int flag)throw(std::runtime_error)
{
	//open in read
	file_no_t::iterator it; 
	//file not buffered
	const file_info *file; 
	if(_file_no.end() ==  (it=_file_no.find(file_path)))
	{
		ssize_t new_file_no=_get_file_no(); 
		if(-1 != new_file_no)
		{
			try
			{
				file_info new_file(file_path, BLOCK_SIZE, _read_from_storage(file_path, new_file_no), flag); 
				_buffered_files.insert(std::make_pair(new_file_no, new_file));
				file=&(_buffered_files.at(new_file_no)); 
				_file_no.insert(std::make_pair(file_path, new_file_no)); 
			}
			catch(std::runtime_error &e)
			{
				throw; 
			}
		}
	}
	//file buffered
	else
	{
		file=&(_buffered_files.at(it->second)); 
	}
	return file->IOnodes; 
}

Master::node_t Master::_read_from_storage(const char* file_path, ssize_t file_no)
{
	size_t file_length=0; 
	size_t block_size=0; 
	node_t nodes=_select_IOnode(file_length, block_size);
	for(node_t::iterator it=nodes.begin(); 
			nodes.end()!=it; ++it)
	{
		//send read request to each IOnode
		//buffer requset
		//file_no
		//file_path
		//start_point
		Send(it->second, BUFFER_FILE);
		Send(it->second, file_no); 
		Sendv(it->second, file_path, strlen(file_path)); 
		Send(it->second, it->first); 
		Send(it->second, block_size); 
	}
	return nodes; 
}

Master::node_t Master::_select_IOnode(size_t file_size, size_t block_size)
{
	//dummy 
	node_t nodes; 
	return nodes; 
}
