#ifndef BB_H_
#define BB_H_

struct file_stat
{
	size_t file_size;
	size_t block_size;
};

class block_info
{
public:
	block_info(const std::string &ip, off_t start_point, size_t size);
	block_info(const char *ip, off_t start_point, size_t size);
		
private:
	std::string ip;
	off_t start_point;
	size_t size;
};


extern "C" 
{
	int open(const char **path, int mode, ...); 

	ssize_t read(int fd, void *buffer, size_t size);

	ssize_t write(int fd, const void *buffer, size_t size);

	int close(int fd);
}

static void _getblock(int socket, size_t size, std::vector<block_info> &block);

#endif
