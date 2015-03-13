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
#include <dirent.h>

#include "Master.h"
#include "Communication.h"
#include "CBB_internal.h"

const char *Master::MASTER_MOUNT_POINT="CBB_MASTER_MOUNT_POINT";

Master::file_info::file_info(const std::string& path,
		ssize_t fileno,
		size_t size,
		size_t block_size,
		const node_t& IOnodes,
		int flag):
	file_no(fileno),
	path(path), 
	p_node(IOnodes), 
	nodes(node_pool_t()),
	size(size),
	block_size(block_size),
	open_count(1),
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
	_current_node_number(0), 
	_current_file_no(0),
	_mount_point(std::string())
{
	memset(_node_id_pool, 0, MAX_NODE_NUMBER*sizeof(bool)); 
	memset(_file_no_pool, 0, MAX_FILE_NUMBER*sizeof(bool)); 
	const char *master_mount_point=getenv(MASTER_MOUNT_POINT);
	
	if(NULL == master_mount_point)
	{
		throw std::runtime_error("please set master mount point");
	}
	_mount_point=std::string(master_mount_point);
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

void Master::_release_file_no(ssize_t file_no)
{
	--_file_number;
	_file_no_pool[file_no]=false;
	return;
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

void Master::_send_node_info(int clientfd, const std::string& ip)const 
{
	_LOG("requery for IOnode info, ip=%s\n", ip.c_str());
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

void Master::_send_file_info(int clientfd, const std::string& ip)const 
{
	ssize_t file_no=0;
	_LOG("requery for File info, ip=%s\n", ip.c_str());
	Recv(clientfd, file_no);

	try
	{
		const file_info &file=_buffered_files.at(file_no);
		Send(clientfd, SUCCESS);
		Sendv(clientfd, file.path.c_str(), file.path.size());
		Send(clientfd, file.size);
		Send(clientfd, file.block_size);
		Send(clientfd, file.flag);
	}
	catch(std::out_of_range &e)
	{
		Send(clientfd, NO_SUCH_FILE);
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
	case GET_ATTR:
		_parse_attr(clientfd, ip);break;
	case READ_DIR:
		_parse_readdir(clientfd, ip);break;
	case RM_DIR:
		_parse_rmdir(clientfd, ip);break;
	case UNLINK:
		_parse_unlink(clientfd, ip);break;
	case ACCESS:
		_parse_access(clientfd, ip);break;
	default:
		Send(clientfd, UNRECOGNISTED); 
		close(clientfd); 
	}
	return ans;
}

int Master::_parse_regist_IOnode(int clientfd, const std::string& ip)
{
	size_t total_memory;
	_LOG("regist IOnode\nip=%s\n",ip.c_str());
	Recv(clientfd, total_memory);
	Send(clientfd, _add_IO_node(ip, total_memory, clientfd));
	_DEBUG("total memory %lu\n", total_memory);
	return 1;
}

int Master::_send_file_meta(int clientfd, const std::string& ip)const
{
	char *path=NULL;
	_LOG("requery for File meta data, ip=%s\n", ip.c_str());
	struct stat buff;
	Recvv(clientfd, &path);
	try
	{
		//const file_info &file=_buffered_files.at(file_no);
		Send(clientfd, SUCCESS);
		Sendv(clientfd, &buff, sizeof(struct stat));
	}
	catch(std::out_of_range &e)
	{
		if(-1 == stat(path, &buff))
		{
			Send(clientfd, NO_SUCH_FILE);
		}
		else
		{
			Send(clientfd, SUCCESS);
			Sendv(clientfd, &buff, sizeof(struct stat));
		}
	}
	free(path);
	close(clientfd);
	return SUCCESS; 
}

void Master::_send_block_info(int clientfd, const node_id_pool_t& node_pool, const node_t& node_set)const
{
	Send(clientfd, static_cast<int>(node_pool.size()));
	for(node_id_pool_t::const_iterator it=node_pool.begin();
			it!=node_pool.end();++it)
	{
		Send(clientfd, *it);
		const std::string& ip=_registed_IOnodes.at(*it).ip;
		Sendv(clientfd, ip.c_str(), ip.size());
	}

	Send(clientfd, static_cast<int>(node_set.size()));

	for(node_t::const_iterator it=node_set.begin(); it!=node_set.end(); ++it)
	{
		Send(clientfd, it->first);
		Send(clientfd, it->second);
	}
	close(clientfd);
	return; 
}	

/*int Master::_parse_node_info(int clientfd, const std::string& ip) const
{
	ssize_t file_no; 
	_DEBUG("requery for File info, ip=%s\n", ip.c_str());
	Recv(clientfd, file_no); 
	try
	{
		file=&(_buffered_files.at(file_no));
		Send(clientfd, SUCCESS);
	}
	catch(std::out_of_range &e)
	{
		//const char OUT_OF_RANGE[]="out of range\n"; 
		Send(clientfd, OUT_OF_RANGE);
		return FAILURE; 
	}
	Send(clientfd, file->size);
	node_id_pool_t node_id;
	_send_block_info(clientfd, node_id, file->p_node);
	close(clientfd);
	return SUCCESS;
}*/

int Master::_parse_attr(int clientfd, const std::string& ip)const
{
	struct stat fstat;
	_DEBUG("requery for File info, ip=%s\n", ip.c_str());
	std::string real_path, relative_path;
	Server::_recv_real_relative_path(clientfd, real_path, relative_path);
	_DEBUG("file path=%s\n", real_path.c_str());
	try
	{
		ssize_t fd=_file_no.at(relative_path);
		_DEBUG("buffered file file_no=%ld\n", fd);
		stat(real_path.c_str(), &fstat);
		_get_buffered_file_attr(fd, &fstat);
		Send(clientfd, SUCCESS);
		Send(clientfd, fstat);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		if(-1 != stat(real_path.c_str(), &fstat))
		{
			Send(clientfd, SUCCESS);
			Send(clientfd, fstat);
			_DEBUG("finished\n");
			return SUCCESS;
		}
		else
		{
			Send(clientfd, errno);
			return FAILURE;
		}
	}
}

int Master::_parse_readdir(int clientfd, const std::string &ip)const
{
	_DEBUG("requery for dir info, ip=%s\n", ip.c_str());
	std::string real_path=Server::_recv_real_path(clientfd);
	_DEBUG("file path=%s\n", real_path.c_str());
	DIR *dir=opendir(real_path.c_str());
	if(NULL != dir)
	{
		const struct dirent* entry=NULL;
		dir_t files;
		while(NULL != (entry=readdir(dir)))
		{
			files.push_back(std::string(entry->d_name));
		}
		Send(clientfd, SUCCESS);
		Send(clientfd, static_cast<int>(files.size()));
		for(dir_t::const_iterator it=files.begin();
				it!=files.end();++it)
		{
			Sendv(clientfd, it->c_str(), it->length());
		}
		closedir(dir);
		return SUCCESS;
	}
	else
	{
		Send(clientfd, errno);
		return FAILURE;
	}
}

int Master::_get_buffered_file_attr(ssize_t fd, struct stat* fstat)const
{
	const file_info& file=_buffered_files.at(fd);
	fstat->st_size=file.size;
	return 1;
}

int Master::_parse_registed_request(int clientfd)
{
	int request, ans=SUCCESS; 
	Recv(clientfd, request); 
	ssize_t id;
	switch(request)
	{
	case I_AM_SHUT_DOWN:
		id=_IOnode_socket.at(clientfd)->node_id;
		_DEBUG("IOnode %ld shutdown\nIP Address=%s, Unregisted\n", id, _registed_IOnodes.at(id).ip.c_str()); 
		_delete_IO_node(clientfd);break;
	default:
		Send(clientfd, UNRECOGNISTED); 
	}
	return ans; 
}

int Master::_parse_open_file(int clientfd, const std::string& ip)
{
	char *file_path=NULL;
	_LOG("request for open file, ip=%s\n", ip.c_str()); 
	Recvv(clientfd, &file_path); 
	int flag ,ret=SUCCESS;
	Recv(clientfd, flag); 
	try
	{
		ssize_t file_no;
		if(flag & O_CREAT)
		{
			mode_t mode;
			Recv(clientfd, mode);
			_create_file(file_path, mode);
			flag &= ~(O_CREAT | O_TRUNC);
		}
		_open_file(file_path, flag, file_no); 
		file_info &opened_file=_buffered_files.at(file_no);
		size_t size=opened_file.size;
		size_t block_size=opened_file.block_size;
		delete file_path; 
		Send(clientfd, SUCCESS);
		Send(clientfd, file_no);
		Send(clientfd, size);
		Send(clientfd, block_size);
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
		_DEBUG("%s\n",e.what());
		Send(clientfd, UNKNOWN_ERROR);
		ret=FAILURE;
	}
	catch(std::invalid_argument &e)
	{
		_DEBUG("%s\n",e.what());
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

void Master::_create_file(const char* file_path, mode_t mode)throw(std::runtime_error)
{
	std::string real_path=_get_real_path(file_path);
	int fd=creat64(real_path.c_str(), mode);
	if(-1 == fd)
	{
		throw std::runtime_error("error on create file");
	}
	else
	{
		close(fd);
		return ;
	}
}

const Master::node_t& Master::_open_file(const char* file_path, int flag, ssize_t& file_no)throw(std::runtime_error, std::invalid_argument, std::bad_alloc)
{
	file_no_t::iterator it; 
	file_info *file=NULL; 
	//file not buffered
	if(_file_no.end() ==  (it=_file_no.find(file_path)))
	{
		file_no=_get_file_no(); 
		if(-1 != file_no)
		{
			/*//open in read
			if(flag == O_RDONLY || flag&O_RDWR)
			{*/
			size_t file_length=0,block_size=0;
			try
			{
				node_t nodes=_send_request_to_IOnodes(file_path, file_no, flag, file_length, block_size);
				file_info new_file(file_path, file_no, file_length, block_size, nodes, flag); 
				_buffered_files.insert(std::make_pair(file_no, new_file));
				file=&(_buffered_files.at(file_no)); 
				_file_no.insert(std::make_pair(file_path, file_no)); 
			}
			catch(std::invalid_argument &e)
			{
				//file open error
				_release_file_no(file_no);
				throw;
			}
			/*}
			else
			{
				size_t file_length=0,block_size=0;
				file_info new_file(file_path, file_length, block_size, node_t(), flag); 
				_buffered_files.insert(std::make_pair(new_file_no, new_file));
				file=&(_buffered_files.at(new_file_no)); 
				_file_no.insert(std::make_pair(file_path, new_file_no)); 
			}*/
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
		++file->open_count;
		file_no=it->second;
	}
	return file->p_node;
}

int Master::_parse_unlink(int clientfd, const std::string& ip)
{
	_LOG("request for unlink, ip=%s\n", ip.c_str()); 
	std::string relative_path;
	std::string real_path;
	Server::_recv_real_relative_path(clientfd, real_path, relative_path);
	_LOG("path=%s\n", real_path.c_str());
	try
	{
		int fd=_file_no.at(relative_path);
		_remove_file(fd);
		Send(clientfd, SUCCESS);
		return SUCCESS;
	}
	catch(std::out_of_range& e)
	{
		if(-1 != unlink(real_path.c_str()))
		{
			Send(clientfd, SUCCESS);
			return SUCCESS;
		}
		else
		{
			Send(clientfd, errno);
			return FAILURE;
		}
	}
}
int Master::_parse_rmdir(int clientfd, const std::string& ip)
{
	_LOG("request for rmdir, ip=%s\n", ip.c_str()); 
	std::string file_path=Server::_recv_real_path(clientfd);
	_LOG("path=%s\n", file_path.c_str());
	if(-1 != rmdir(file_path.c_str()))
	{
		Send(clientfd, SUCCESS);
		return SUCCESS;
	}
	else
	{
		Send(clientfd, errno);
		return FAILURE;
	}
}

Master::node_t Master::_send_request_to_IOnodes(const char *file_path, ssize_t file_no, int flag, size_t& file_length,size_t& block_size)throw(std::invalid_argument)
{
	std::string real_path=_get_real_path(file_path);
	_DEBUG("file path=%s\n", real_path.c_str());
	int fd=open64(real_path.c_str(), flag);
	
	if(-1 == fd)
	{
		throw std::invalid_argument("file can not open");
	}

	struct stat file_stat;
	fstat(fd, &file_stat);
	file_length=file_stat.st_size;
	block_size=_get_block_size(file_length); 
	close(fd);
	node_block_map_t node_block_map;
	node_t nodes=_select_IOnode(0, file_length, block_size, node_block_map);
	for(node_block_map_t::const_iterator it=node_block_map.begin(); 
			node_block_map.end()!=it; ++it)
	{
		//send read request to each IOnode
		//buffer requset, file_no, file_path, start_point, block_size
		int socket=_registed_IOnodes.at(it->first).socket;
		Send(socket, OPEN_FILE);
		Send(socket, file_no); 
		Send(socket, flag);
		Sendv(socket, file_path, strlen(file_path));
		
		Send(socket, static_cast<int>(it->second.size()));
		for(block_info_t::const_iterator blocks_it=it->second.begin();
			blocks_it!=it->second.end(); ++blocks_it)
		{
			Send(socket, blocks_it->first);
			Send(socket, blocks_it->second);
		}
	}
	return nodes; 
}

inline off64_t Master::_get_block_start_point(off64_t start_point, size_t& size)const
{
	off64_t block_start_point=(start_point/BLOCK_SIZE)*BLOCK_SIZE;
	size=start_point-block_start_point+size;
	return block_start_point;
}

Master::node_t Master::_select_IOnode(off64_t start_point,
		size_t file_size,
		size_t block_size,
		node_block_map_t& node_block_map)const
{
	off64_t block_start_point=_get_block_start_point(start_point, file_size);
	//int node_number = (file_size+block_size-1)/block_size,count=(node_number+_node_number-1)/_node_number, count_node=0;
	int node_number = (file_size+block_size-1)/block_size,count=(node_number+_node_number-1)/_node_number, count_node=0;
	node_t nodes;
	size_t remaining_size=file_size;
	if(0 == count && 0 == node_number)
	{
		count=1;
		node_number=1;
	}
	for(IOnode_t::const_iterator it=_registed_IOnodes.begin();
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

int Master::_parse_read_file(int clientfd, const std::string& ip)
{
	ssize_t file_no;
	size_t size;
	off64_t start_point;
	file_info *file=NULL; 
	_LOG("request for reading ip=%s\n",ip.c_str());
	Recv(clientfd, file_no); 
	try
	{
		file=&(_buffered_files.at(file_no));
		Send(clientfd, SUCCESS);
	}
	catch(std::out_of_range &e)
	{
		Send(clientfd, OUT_OF_RANGE);
		return -1; 
	}
	Send(clientfd, file->size);
	
	Recv(clientfd, start_point);
	Recv(clientfd, size);
	//_prepare_read_block(*file, start_point, size);
	node_t nodes;
	node_id_pool_t node_pool;
	if(file->size < start_point+size)
	{
		if(static_cast<size_t>(start_point) > file->size)
		{
			size=0;
		}
		else
		{
			size=file->size-start_point;
		}
	}
	Send(clientfd, size);
	_get_IOnodes_for_IO(start_point, size, *file, nodes, node_pool);
	//_send_IO_request(file_no, *file, nodes, size, READ_FILE);
	_send_block_info(clientfd, node_pool, nodes);
	close(clientfd);
	return 1;
}

void Master::_send_IO_request(ssize_t file_no, const file_info& file, const node_t& node_set, size_t size, int mode)const
{
	/*for(node_t::const_iterator it=node_set.begin(); 
			node_set.end()!=it; ++it)
	{
		//send read request to each IOnode
		//buffer requset, file_no, file_path, start_point, block_size
		int socket=_registed_IOnodes.at(it->second).socket;
		size_t my_block_size=file.block_size;//it->first> file.size?file.size-it->first;
		Send(socket, WRITE_FILE);
		Send(socket, file_no);
		Sendv(socket, file.path.c_str(), file.path.size());
		Send(socket, it->first); 
		Send(socket, my_block_size); 
	}*/
	int socket=_registed_IOnodes.at(node_set.begin()->second).socket;
	Send(socket, mode);
	Send(socket, file_no);
	//total I/O size
	Send(socket, size);
	Send(socket, static_cast<int>(node_set.size()));
	for(node_t::const_iterator it=node_set.begin();
			node_set.end()!=it; ++it)
	{
		Send(socket, it->first); 
	}

	return;
}

int Master::_allocate_one_block(const struct file_info& file)throw(std::bad_alloc)
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

void Master::_append_block(struct file_info& file, int node_id, off64_t start_point)
{
	file.p_node.insert(std::make_pair(start_point, node_id));
	file.nodes.insert(node_id);
}

void Master::_send_append_request(ssize_t file_no, const node_block_map_t& append_blocks)const
{
	for(node_block_map_t::const_iterator nodes_it=append_blocks.begin();
			nodes_it != append_blocks.end();++nodes_it)
	{
		int socket = _registed_IOnodes.at(nodes_it->first).socket;
		int ret;
		Send(socket, APPEND_BLOCK);
		Send(socket, file_no);
		Recv(socket, ret);
		if(SUCCESS == ret)
		{
			Send(socket, static_cast<int>(nodes_it->second.size()));
			for(block_info_t::const_iterator block_it=nodes_it->second.begin();
					block_it != nodes_it->second.end();++block_it)
			{
				Send(socket, block_it->first);
				Send(socket, block_it->second);
			}
		}
	}
	return;
}

Master::node_t& Master::_get_IOnodes_for_IO(off64_t start_point, size_t &size, struct file_info& file, node_t& node_set, node_id_pool_t& node_id_pool)throw(std::bad_alloc)
{
	off64_t current_point=_get_block_start_point(start_point, size);
	ssize_t remaining_size=size;
	node_t::iterator it;
	node_block_map_t node_append_block;
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
				node_append_block[node_id].insert(std::make_pair(current_point, IO_size));
			}
			catch(std::bad_alloc& e)
			{
				throw;
			}
		}
	}
	_send_append_request(file.file_no, node_append_block);
	return node_set;
}

int Master::_parse_write_file(int clientfd, const std::string& ip)
{
	_LOG("request for writing ip=%s\n",ip.c_str());
	ssize_t file_no;
	size_t size;
	off64_t start_point;
	Recv(clientfd, file_no);
	//Recv(clientfd, mode); 
	try
	{
		file_info &file=_buffered_files.at(file_no);
		//file.block_size=_get_block_size(size);
		std::string real_path=_get_real_path(file.path);
		int fd=open64(real_path.c_str(), O_CREAT, 0600);
		if(-1 == fd)
		{
			_LOG("file create error\n");
			Send(clientfd, errno);
			return FAILURE;
		}
		close(fd);
		Send(clientfd, SUCCESS);
		Send(clientfd, file.size);
		Recv(clientfd, start_point);
		Recv(clientfd, size);
		if(file.size < start_point+size)
		{
			file.size=start_point+size;
		}
		
		//node_t nodes=_select_IOnode(start_point, size, file.block_size);
		node_t nodes;
		node_id_pool_t node_pool;
		_DEBUG("here\n");
		Send(clientfd, size);
		_get_IOnodes_for_IO(start_point, size, file, nodes, node_pool);
		//insert allocate node into file node list
		/*for(node_t::const_iterator it=nodes.begin();
				it!=nodes.end();++it)
		{
			file.p_node.insert(std::make_pair(it->first, it->second));
		}*/

		//_send_IO_request(file_no, file, nodes, size, WRITE_FILE);
		_send_block_info(clientfd, node_pool, nodes);
		close(clientfd);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		Send(clientfd, errno);	
		close(clientfd);
		return FAILURE;
	}
}

int Master::_parse_access(int clientfd, const std::string& ip)const
{
	int mode;
	_LOG("request for access, ip=%s\n", ip.c_str()); 
	std::string file_path=Server::_recv_real_path(clientfd);	
	Recv(clientfd, mode);
	_LOG("path=%s\n, mode=%d", file_path.c_str(), mode);
	if(-1 != access(file_path.c_str(), mode))
	{
		_LOG("SUCCESS\n");
		Send(clientfd, SUCCESS);
		return SUCCESS;
	}
	else
	{
		Send(clientfd, errno);
		return FAILURE;
	}
}

size_t Master::_get_block_size(size_t size)
{
	return BLOCK_SIZE;
	//return (size+_node_number-1)/_node_number;
}

int Master::_parse_flush_file(int clientfd, const std::string& ip)
{
	_LOG("request for writing ip=%s\n",ip.c_str());
	ssize_t file_no;
	Recv(clientfd, file_no);
	try
	{
		file_info &file=_buffered_files.at(file_no);
		Send(clientfd, SUCCESS);
		for(node_pool_t::iterator it=file.nodes.begin();
				it != file.nodes.end();++it)
		{
			_DEBUG("write back request to IOnode %ld, file_no %ld\n", *it, file_no);
			int socket=_IOnode_socket.at(*it)->socket;
			Send(socket, FLUSH_FILE);
			Send(socket, file_no);
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

int Master::_remove_file(int file_no)
{
	file_info &file=_buffered_files.at(file_no);
	for(node_pool_t::iterator it=file.nodes.begin();
			it != file.nodes.end();++it)
	{
		int socket=_registed_IOnodes.at(*it).socket;
		Send(socket, CLOSE_FILE);
		Send(socket, file_no);
	}
	std::string &path=file.path;
	_file_no.erase(path);
	_buffered_files.erase(file_no);
	return SUCCESS;
}

int Master::_parse_close_file(int clientfd, const std::string& ip)
{
	_LOG("request for closing file ip=%s\n",ip.c_str());
	ssize_t file_no;
	Recv(clientfd, file_no);
	file_info &file=_buffered_files.at(file_no);
	_LOG("file no %ld\n", file_no);
	try
	{
		Send(clientfd, SUCCESS);
		close(clientfd);
		//if(0 == --file.open_count)
		//{

			for(node_pool_t::iterator it=file.nodes.begin();
					it != file.nodes.end();++it)
			{
				_DEBUG("write back request to IOnode %ld\n", *it);
				int socket=_registed_IOnodes.at(*it).socket;
				Send(socket, FLUSH_FILE);
				//Send(socket, CLOSE_FILE);
				Send(socket, file_no);
				//			int ret=0;
				//			Recv(socket, ret);
			}
			//File_t::iterator file=_buffered_files.find(file_no);
			//std::string &path=file->second.path;
			//_file_no.erase(path);
			//_buffered_files.erase(file);
		//}
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		Send(clientfd, FAILURE);
		close(clientfd);
		return FAILURE;
	}
}

inline std::string Master::_get_real_path(const char* path)const
{
	return _mount_point+std::string(path);
}

inline std::string Master::_get_real_path(const std::string& path)const
{
	return _mount_point+path;
}
