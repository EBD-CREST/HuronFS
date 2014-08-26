#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <aio.h>

#include "include/IO_const.h"
#include "include/Communication.h"
#include "include/User_Client.h"

User_Client::User_Client(const std::string& master_ip, int client_port)throw(std::runtime_error):
	Client(),
	_opened_file(file_point_t())
{
	memset(&client_addr, 0, sizeof(client_addr)); 
	memset(&master_addr, 0, sizeof(master_addr)); 
	memset(&IOnode_addr, 0, sizeof(IOnode_addr)); 
	client_addr.sin_family = AF_INET; 
	client_addr.sin_addr.s_addr = htons(INADDR_ANY); 
	client_addr.sin_port = htons(client_port); 
	master_addr.sin_family = AF_INET; 
	master_addr.sin_port = htons(MASTER_PORT);
	IOnode_addr.sin_family = AF_INET; 
	IOnode_addr.sin_port = htons(IONODE_PORT);
	if(0 == inet_aton(master_ip.c_str(), &master_addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error"); 
	}
}

User_Client::~User_Client()
{
	for(file_point_t::iterator it=_opened_file.begin();
			it!=_opened_file.end();++it)
	{
		iob_close(it->first);
	}
}

int User_Client::iob_fstat(ssize_t fd, struct file_stat &stat)
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
}

ssize_t User_Client::iob_open(const char* path, int flag)
{
	int master_socket=Client::_connect_to_server(client_addr, master_addr); 
	Send(master_socket, OPEN_FILE); 
	Sendv(master_socket, path, strlen(path));
	Send(master_socket, flag); 
	int ret;
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		ssize_t file_no;
		size_t size, block_size;
		Recv(master_socket, file_no);
		Recv(master_socket, size);
		Recv(master_socket, block_size);
		_opened_file.insert(std::make_pair(file_no, static_cast<off_t>(0)));
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

ssize_t User_Client::iob_read(ssize_t fd, void *buffer, size_t size)
{
	int master_socket=Client::_connect_to_server(client_addr, master_addr); 
	Send(master_socket, READ_FILE);
	int ret;
	Send(master_socket, fd);
	Recv(master_socket, ret);
	off_t now_point=0;
	try
	{
		now_point=_opened_file.at(fd);
	}
	catch(std::out_of_range &e)
	{
		fprintf(stderr, "open file first\n");
		return -1;
	}
	if(SUCCESS == ret)
	{
		size_t file_size, block_size;
		Recv(master_socket, file_size);
		Recv(master_socket, block_size);
		int count;
		Recv(master_socket, count);
		off_t start_point;
		char *ip;
		struct aiocb *ionode_aiocb=new aiocb[count]; 
		struct aiocb **aio_list=new aiocb*[count];
		memset(ionode_aiocb,0,sizeof(ionode_aiocb));
		int *socket_pool=new int[count];
		for(int i=0;i<count;++i)
		{
			Recv(master_socket, start_point);
			Recvv(master_socket, &ip);
			off_t end_point=(start_point+block_size <= file_size? start_point+block_size:file_size);
			if(end_point > now_point)
			{
				_set_IOnode_addr(ip);
				socket_pool[i] = Client::_connect_to_server(client_addr, IOnode_addr);
				Send(socket_pool[i], READ_FILE);
				Send(socket_pool[i], fd);
				Send(socket_pool[i], now_point);
				size_t receive_size=end_point-now_point;
				Send(socket_pool[i], receive_size);
				fprintf(stderr, "receive data from %s\nstart_point=%lu, receive_size=%lu\n", ip, now_point, receive_size);
				ionode_aiocb[i].aio_fildes=socket_pool[i]; 
				ionode_aiocb[i].aio_buf=reinterpret_cast<char*>(buffer)+start_point; 
				ionode_aiocb[i].aio_nbytes=receive_size;
				ionode_aiocb[i].aio_lio_opcode=LIO_READ;
				aio_list[i]=ionode_aiocb+i;
				//Recvv_pre_alloc(socket_pool[i], reinterpret_cast<char*>(buffer)+start_point, receive_size);
				now_point=end_point;
			}
			delete ip;
		}
		close(master_socket);
		_do_io(aio_list, count);
		for(int i=0;i<count;++i)
		{
			close(socket_pool[i]);
		}
		delete[] ionode_aiocb;
		delete[] socket_pool;
		delete[] aio_list;
		return size;
	}
	else
	{
		fprintf(stderr, "open file first\n");
		close(master_socket);
		return -1;
	}
}

void User_Client::_do_io(struct aiocb **aio_list, int count)
{
	bool flag=true;
	while(flag)
	{
		flag=false;
		lio_listio(LIO_WAIT, aio_list,count, NULL);	
		for(int i=0;i<count;++i)
		{
			ssize_t ret=aio_return(aio_list[i]);
			if(-1 != ret)
			{
				aio_list[i]->aio_buf+=ret;
				aio_list[i]->aio_nbytes-=ret;
				if(0 != ret && 0 != aio_list[i]->aio_nbytes)
				{
					flag=true;
				}
			}
		}
	}
	return;
}

ssize_t User_Client::iob_write(ssize_t fd, const void *buffer, size_t count)
{
	off_t now_point=0;
	try
	{
		now_point=_opened_file.at(fd);
	}
	catch(std::out_of_range &e)
	{
		fprintf(stderr, "open file first\n");
		return -1;
	}
	int master_socket=Client::_connect_to_server(client_addr, master_addr); 
	Send(master_socket, WRITE_FILE);
	int ret;
	Send(master_socket, fd);
	Send(master_socket, now_point);
	Send(master_socket, count);
	Recv(master_socket, ret);
	if(SUCCESS == ret)
	{
		int count;
		Recv(master_socket, count);
		off_t start_point;
		size_t file_size, block_size;
		Recv(master_socket, file_size);
		Recv(master_socket, block_size);
		char *ip;
		struct aiocb *ionode_aiocb=new aiocb[count]; 
		struct aiocb **aio_list=new aiocb*[count];
		memset(ionode_aiocb, 0, sizeof(struct aiocb));
		int *socket_pool=new int[count];
		for(int i=0;i<count;++i)
		{
			Recv(master_socket, start_point);
			Recvv(master_socket, &ip);
			_set_IOnode_addr(ip);
			socket_pool[i] = Client::_connect_to_server(client_addr, IOnode_addr);
			Send(socket_pool[i], WRITE_FILE);
			Send(socket_pool[i], fd);
			Send(socket_pool[i], start_point);
			size_t send_size=(start_point+block_size < file_size? block_size:file_size-start_point);
			Send(socket_pool[i], send_size);
			fprintf(stderr, "send data to %s\nstart_point=%lu\n", ip, start_point);
			ionode_aiocb[i].aio_fildes=socket_pool[i]; 
			ionode_aiocb[i].aio_offset=0;
			ionode_aiocb[i].aio_buf=reinterpret_cast<char*>(const_cast<void*>(buffer))+start_point; 
			ionode_aiocb[i].aio_nbytes=send_size;
			ionode_aiocb[i].aio_lio_opcode=LIO_WRITE;
			aio_list[i]=ionode_aiocb+i;
			//Sendv_pre_alloc(socket_pool[i], reinterpret_cast<const char*>(buffer)+start_point, send_size);
			now_point+=send_size;
			delete ip;
		}
		close(master_socket);
		_do_io(aio_list, count);
		for(int i=0;i<count;++i)
		{
			close(socket_pool[i]);
		}
		delete[] ionode_aiocb;
		delete[] socket_pool;
		delete[] aio_list;
		return count;
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
