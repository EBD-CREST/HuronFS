#ifndef CBB_H_

#define CBB_H_

#include <map>
#include <vector>
#include <netinet/in.h>

#include "Client.h"
#include "CBB_const.h"


class CBB:Client
{

private:
	class  block_info
	{
	public:
		block_info(ssize_t node_id, off64_t start_point, size_t size);
		ssize_t node_id;
		off64_t start_point;
		size_t size;
	};
	
	class file_info
	{
	public:
		file_info(ssize_t file_no, int fd, size_t size, size_t block_size, int flag);
		file_info();
		ssize_t file_no;
		off64_t current_point;
		int fd;
		size_t size;
		size_t block_size;
		int flag;
	};

public:
	//map fd file_info
	typedef std::map<int, file_info> _file_list_t;
	typedef std::vector<bool> _file_t;
	typedef std::vector<block_info> _block_list_t;
	typedef std::map<ssize_t, std::string> _node_pool_t;
	static const char *CLIENT_MOUNT_POINT;
	static const char *MASTER_IP;

public:
	//initalize parameters
	CBB();
	~CBB();
	//posix API
	int _open(const char *path, int flag, mode_t mode);

	ssize_t _read(int fd, void *buffer, size_t size);

	ssize_t _write(int fd,const void *buffer, size_t size);

	int _close(int fd);

	int _flush(int fd);

	off64_t _lseek(int fd, off64_t offset, int whence);

	int _fstat(int fd, struct stat* buf);
	
	off64_t _tell(int fd);

	//int _stat(std::string true_path, struct stat* buf);
	
	static bool _interpret_path(const char* path);
	static bool _interpret_fd(int fd);
	static void _format_path(const char* path, std::string &formatted_path);
	static void _get_true_path(const std::string& formatted_path, std::string &true_path);
	size_t _get_file_size(int fd);
private:
	//private functions
	void _getblock(int socket, off64_t start_point, size_t& size, std::vector<block_info> &block, _node_pool_t &node_pool);
	ssize_t _read_from_IOnode(file_info& file, const _block_list_t& blocks, const _node_pool_t& node_pool, char *buffer, size_t size);
	ssize_t _write_to_IOnode(file_info& file, const _block_list_t& blocks, const _node_pool_t& node_pool, const char *buffer, size_t size);

	int _get_fid();
	
	static int _BB_fd_to_fid(int fd);
	static int _BB_fid_to_fd(int fid);
private:
	int _fid_now;
	_file_list_t _file_list;
	_file_t _opened_file;
	struct sockaddr_in _master_addr;
	struct sockaddr_in _client_addr;
	bool _initial;
};


#endif
