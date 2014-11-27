#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <map>
#include <vector>

#include "include/IO_const.h"
#include "include/Communication.h"
#include "include/BB.h"
#include "include/User_Client.h"

/*int ::iob_fstat(ssize_t fd, struct file_stat &stat)
{
	int master_socket=Client::_connect_to_server(client_addr, master_addr); 
	Send(master_socket, GET_FILE_META); 
	Send(master_socket, fd); 
	int ret;
	Recv(master_socket, ret);
	if(ret == SUCCESS)
	{
		Recv(master_socket, stat.file_size);
		Recv(master_socket, stat.block_size);
	}
	return ret;
}*/
User_client client;


block_info::block_info(const std::string &ip, off_t start_point, size_t size):
	ip(ip),
	start_point(start_point),
	size(size)
{}

static void _getblock(int socket, size_t size, std::vector<block_info> &block)
{
	char *ip=NULL;
	
	Recv(socket, file_size);
	Recv(socket, block_size);
	Recv(socket, count);
	for(int i=0;i<count;++i)
	{
		Recv(master_socket, start_point);
		Recvv(master_socket, &ip);
		off_t end_point=(start_point+block_size <= file_size? start_point+block_size:file_size);
		blocks(block_info(ip, start_point, size);
		free(ip);
	}
	close(master_socket);
}
	
extern "C" int open(const char* path, int flag, ...)
{
	int master_socket=client.Client::_connect_to_server(client_addr, master_addr); 
	va_list ap;
	ssize_t file_no;
	size_t size, block_size;
	int ret;
	mode_t mode;
	Send(master_socket, OPEN_FILE); 
	Sendv(master_socket, path, strlen(path));
	Send(master_socket, flag); 
	if(mode & O_CREAT)
	{
		va_start(ap, flag);
		mode=va_arg(ap, mode_t);
		va_end(ap);
		Send(master_Socket, mode);
	}
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		Recv(master_socket, file_no);
		Recv(master_socket, size);
		Recv(master_socket, block_size);
		close(master_socket);
		return file_no;
	}
	else
	{
		fprintf(stderr, "File Open Error\n");
		close(master_socket);
		return -1;
	}
}

ssize_t read(int fd, void *buffer, size_t size)
{
	int master_socket=client.Client::_connect_to_server(client_addr, master_addr); 
	int ret;
	off_t start_point;
	char *tmp=NULL;
	off_t now_point=0;
	int count;
	size_t file_size, block_size;
	ssize_t ans;
	
	vector<block_info> blocks;
	Send(master_socket, READ_FILE);
	Send(master_socket, fd);
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		_getblock(master_socket, size, block);
		for(int i=0;i<count;++i)
		{
			if(end_point > now_point)
			{
				socket = Client::_connect_to_server(client_addr, IOnode_addr);
				Send(socket, READ_FILE);
				Send(socket, fd);
				Send(socket, now_point);
				Send(socket, receive_size);
				fprintf(stderr, "receive data from %s\nstart_point=%lu, receive_size=%lu\n", ip, now_point, receive_size);

				ssize_t ret_size;
				ret_size=Recvv_pre_alloc(socket, tmp, receive_size);
				tmp+=ret_size;
				ans+=ret_size;
			}
		}
		return ans;
	}
	else
	{
		fprintf(stderr, "open file first\n");
		close(master_socket);
		return -1;
	}
}

ssize_t read(int fd, void *buffer, size_t size)
{
	int master_socket=client.Client::_connect_to_server(client_addr, master_addr); 
	int ret;
	off_t start_point;
	char *ip=NULL, *tmp=NULL;
	off_t now_point=0;
	int count;
	size_t file_size, block_size;
	ssize_t ans;
	
	vector<block_info> blocks;
	Send(master_socket, WRITE_FILE);
	Send(master_socket, fd);
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{

		_getblock(master_socket, size, block);
		for(int i=0;i<count;++i)
		{
			if(end_point > now_point)
			{
				socket = Client::_connect_to_server(client_addr, IOnode_addr);
				Send(socket, WRITE_FILE);
				Send(socket, fd);
				Send(socket, now_point);
				Send(socket, receive_size);
				fprintf(stderr, "receive data from %s\nstart_point=%lu, receive_size=%lu\n", ip, now_point, receive_size);

				ssize_t ret_size;
				ret_size=Recvv_pre_alloc(socket, tmp, receive_size);
				tmp+=ret_size;
				ans+=ret_size;
			}
		}
		return ans;
	}
	else
	{
		fprintf(stderr, "open file first\n");
		close(master_socket);
		return -1;
	}
}

void User_Client::_set_IOnode_addr(const char* ip)throw(std::runtime_error)
{
	if(0  == inet_aton(ip, &IOnode_addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error"); 
	}
}

int User_Client::iob_flush(ssize_t fd)
{
	int master_socket=Client::_connect_to_server(client_addr, master_addr); 
	Send(master_socket, FLUSH_FILE);
	Send(master_socket, fd);
	int ret=SUCCESS;
	Recv(master_socket, ret);
	return ret;
}

int User_Client::iob_close(ssize_t fd)
{
	int master_socket=Client::_connect_to_server(client_addr, master_addr); 
	Send(master_socket, CLOSE_FILE);
	Send(master_socket, fd);
	int ret=SUCCESS;
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		_opened_file.erase(fd);
	}
	return ret;
}

