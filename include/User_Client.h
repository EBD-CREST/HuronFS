#ifndef QUERY_CLIENT_H_
#define QUERY_CLIENT_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <map>

#include "include/Client.h"

struct file_stat
{
	size_t file_size;
	size_t block_size;
};

class User_Client:public Client
{
public:
	User_Client(const std::string &master_ip, int port)throw(std::runtime_error); 
	~User_Client();
	ssize_t iob_open(const char* path, int flag); 
	ssize_t iob_read(ssize_t fd, void *buffer, size_t count); 
	ssize_t iob_write(ssize_t fd, const void *buffer, size_t count); 
	int iob_flush(ssize_t fd);
	int iob_close(ssize_t fd);
	int iob_fstat(ssize_t fd, struct file_stat& stat);
	void parse_query();
private:
	User_Client(const User_Client&); 
	void _command();
	void _set_IOnode_addr(const char *ip)throw(std::runtime_error);
	void _do_io(struct aiocb ** aiocb_list, int count);
private:
	typedef std::map<ssize_t, off_t> file_point_t;
private:

	file_point_t _opened_file;
	struct sockaddr_in master_addr; 
	struct sockaddr_in IOnode_addr;
	struct sockaddr_in client_addr; 
};


#endif
