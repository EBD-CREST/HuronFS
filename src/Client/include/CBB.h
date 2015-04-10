#ifndef CBB_H_

#define CBB_H_

#include <map>
#include <vector>
#include <netinet/in.h>
#include <string>
#include <sys/stat.h>
#include <set>

#include "Client.h"
#include "CBB_const.h"


class CBB:Client
{
private:
	class block_info;
	class file_meta;
	class file_info;
	//map path, fd
	typedef std::map<std::string, file_meta*> _path_file_meta_map_t;

public:
	typedef std::set<int> opened_fd_t;
private:

	class  block_info
	{
	public:
		friend class CBB;
		block_info(ssize_t node_id, off64_t start_point, size_t size);
	private:
		ssize_t node_id;
		off64_t start_point;
		size_t size;
	};

	class file_meta
	{
	public:
		friend class CBB;
		friend class file_info;
		file_meta(ssize_t file_no,
				size_t block_size,
				const struct stat* file_stat,
				int master_socket);
	private:
		ssize_t file_no;
		int open_count;
		size_t block_size;
		struct stat file_stat;
		opened_fd_t opened_fd;
		int master_socket;
		_path_file_meta_map_t::iterator it;

	};

	class file_info
	{
	public:
		friend class CBB;
		file_info(int fd, int flag, file_meta* file_meta_p);
		file_info();
		file_info(const file_info& src);
		~file_info();
	private:
		const file_info& operator=(const file_info&);
		off64_t current_point;
		int fd;
		int flag;
		file_meta* file_meta_p;
	};

public:
	//map fd file_info
	typedef std::map<int, file_info> _file_list_t;
	typedef std::vector<bool> _file_t;
	typedef std::vector<block_info> _block_list_t;
	typedef std::map<ssize_t, std::string> _node_pool_t;
	typedef std::vector<std::string> dir_t;
	//map IOnode id: fd
	typedef std::map<int, int> IOnode_fd_map_t;
	typedef std::vector<int> master_list_t;

	static const char *CLIENT_MOUNT_POINT;
	static const char *MASTER_IP_LIST;

public:
	//initalize parameters
	CBB();
	virtual ~CBB();
	//posix API
	int _open(const char *path, int flag, mode_t mode);
	ssize_t _read(int fd, void *buffer, size_t size);
	ssize_t _write(int fd,const void *buffer, size_t size);
	int _close(int fd);
	int _flush(int fd);
	off64_t _lseek(int fd, off64_t offset, int whence);
	int _fstat(int fd, struct stat* buf);
	int _getattr(const char *path, struct stat* fstat);
	int _readdir(const char* path, dir_t& dir)const;
	int _unlink(const char* path);
	int _rmdir(const char* path);
	int _access(const char* path, int mode);
	int _stat(const char* path, struct stat* buf);
	int _rename(const char* old_name, const char* new_name);
	int _mkdir(const char* path, mode_t mode);
	int _touch(int fd);
	int _truncate(const char*path, off64_t size);
	int _ftruncate(int fd, off64_t size);
	
	off64_t _tell(int fd);

	//int _stat(std::string true_path, struct stat* buf);
	
	static bool _interpret_path(const char* path);
	static bool _interpret_fd(int fd);
	static void _format_path(const char* path, std::string &formatted_path);
	static void _get_relative_path(const std::string& formatted_path, std::string &true_path);
	size_t _get_file_size(int fd);
	int _get_fd_from_path(const char* path);
	int _update_file_size(int fd, size_t size);
	int _write_update_file_size(file_info& file, size_t size);
private:
	//private functions
	void _getblock(int socket, off64_t start_point, size_t& size, std::vector<block_info> &block, _node_pool_t &node_pool);
	ssize_t _read_from_IOnode(file_info& file, const _block_list_t& blocks, const _node_pool_t& node_pool, char *buffer, size_t size);
	ssize_t _write_to_IOnode(file_info& file, const _block_list_t& blocks, const _node_pool_t& node_pool, const char *buffer, size_t size);

	int _get_fid();
	int _regist_to_master();
	int _get_IOnode_socket(int IOnodeid, const std::string& ip);
	
	static inline int _BB_fd_to_fid(int fd);
	static inline int _BB_fid_to_fd(int fid);
	int _update_fstat_to_server(file_info& file);
	int _get_local_attr(const char*path, struct stat *file_stat);
	file_meta* _create_new_file(int master_socket);
	int _close_local_opened_file(const char* path);
	int _get_master_socket_from_path(const std::string& path)const;
	int _get_master_socket_from_fd(int fd)const;
private:
	int _fid_now;
	_file_list_t _file_list;
	_file_t _opened_file;
	_path_file_meta_map_t _path_file_meta_map;
	struct sockaddr_in _client_addr;
	bool _initial;

	master_list_t master_socket_list;
	IOnode_fd_map_t IOnode_fd_map;
};


#endif
