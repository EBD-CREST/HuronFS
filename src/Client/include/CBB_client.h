#ifndef CBB_CLIENT_H_

#define CBB_CLIENT_H_

#include <vector>
#include <netinet/in.h>
#include <string>
#include <sys/stat.h>
#include <set>
#include <map>

#include "Client.h"
#include "CBB_const.h"

namespace CBB
{
	namespace Client
	{
		class CBB_client:Common::Client
		{
			private:
				class block_info;
				class file_meta;
				class opened_file_info;
				//map path, fd
				typedef std::map<std::string, file_meta*> _path_file_meta_map_t;

			public:
				typedef std::set<int> _opened_fd_t;
			private:

				class  block_info
				{
					public:
						friend class CBB_client;
						//block_info(ssize_t node_id, off64_t start_point, size_t size);
						block_info(off64_t start_point, size_t size);
					private:
						//ssize_t node_id;
						off64_t start_point;
						size_t size;
				};

				class file_meta
				{
					public:
						friend class CBB_client;
						friend class opened_file_info;
						file_meta(ssize_t file_no,
								size_t block_size,
								const struct stat* file_stat,
								int master_number,
								int master_socket);
					private:
						ssize_t file_no;
						int open_count;
						size_t block_size;
						struct stat file_stat;
						_opened_fd_t opened_fd;
						int master_number;
						int master_socket;
						_path_file_meta_map_t::iterator it;

				};

				class opened_file_info
				{
					public:
						friend class CBB_client;
						opened_file_info(int fd, int flag, file_meta* file_meta_p);
						opened_file_info();
						opened_file_info(const opened_file_info& src);
						~opened_file_info();
					private:
						const opened_file_info& operator=(const opened_file_info&);
						//current offset in file
						off64_t current_point;
						int fd;
						int flag;
						file_meta* file_meta_p;
				};

			public:
				//map fd opened_file_info
				typedef std::map<int, opened_file_info> _file_list_t;
				typedef std::vector<bool> _file_t;
				typedef std::vector<block_info> _block_list_t;
				typedef std::map<ssize_t, std::string> _node_pool_t;
				typedef std::set<std::string> dir_t;
				//map IOnode id: fd
				typedef std::map<ssize_t, int> IOnode_fd_map_t;
				typedef std::vector<int> master_list_t;

				static const char *CLIENT_MOUNT_POINT;
				static const char *MASTER_IP_LIST;

			public:
				//initalize parameters
				CBB_client();
				virtual ~CBB_client();
				void start_threads();
				//posix API
			public:
				int open(const char *path, int flag, mode_t mode);
				ssize_t read(int fd, void *buffer, size_t size);
				ssize_t write(int fd,const void *buffer, size_t size);
				int close(int fd);
				int flush(int fd);
				off64_t lseek(int fd, off64_t offset, int whence);
				int getattr(const char *path, struct stat* fstat);
				int readdir(const char* path, dir_t& dir);
				int unlink(const char* path);
				int rmdir(const char* path);
				int access(const char* path, int mode);
				int stat(const char* path, struct stat* buf);
				int rename(const char* old_name, const char* new_name);
				int mkdir(const char* path, mode_t mode);
				int truncate(const char*path, off64_t size);
				int ftruncate(int fd, off64_t size);
				off64_t tell(int fd);

			private:
				int _open(const char *path, int flag, mode_t mode)throw(std::runtime_error);
				ssize_t _read(int fd, void *buffer, size_t size)throw(std::runtime_error);
				ssize_t _write(int fd,const void *buffer, size_t size)throw(std::runtime_error);
				int _close(int fd)throw(std::runtime_error);
				int _flush(int fd)throw(std::runtime_error);
				off64_t _lseek(int fd, off64_t offset, int whence);
				int _getattr(const char *path, struct stat* fstat)throw(std::runtime_error);
				int _readdir(const char* path, dir_t& dir)throw(std::runtime_error);
				int _unlink(const char* path)throw(std::runtime_error);
				int _rmdir(const char* path)throw(std::runtime_error);
				int _access(const char* path, int mode)throw(std::runtime_error);
				int _stat(const char* path, struct stat* buf)throw(std::runtime_error);
				int _rename(const char* old_name, const char* new_name)throw(std::runtime_error);
				int _mkdir(const char* path, mode_t mode)throw(std::runtime_error);
				int _truncate(const char*path, off64_t size)throw(std::runtime_error);
				int _ftruncate(int fd, off64_t size)throw(std::runtime_error);
				off64_t _tell(int fd);

			public:
				int _update_access_time(int fd);
				static bool _interpret_path(const char* path);
				static bool _interpret_fd(int fd);
				static void _format_path(const char* path, std::string &formatted_path);
				static void _get_relative_path(const std::string& formatted_path, std::string &true_path);
				size_t _get_file_size(int fd);
				int _update_file_size(int fd, size_t size);
				int _write_update_file_size(opened_file_info& file, size_t size);
			private:
				//private functions
				void _get_blocks_from_master(Common::extended_IO_task* response,
						off64_t start_point,
						size_t& size,
						std::vector<block_info> &block,
						_node_pool_t &node_pool);

				ssize_t _read_from_IOnode(int master_number,
						opened_file_info& file,
						const _block_list_t& blocks,
						const _node_pool_t& node_pool,
						char *buffer,
						size_t size);

				ssize_t _write_to_IOnode(int master_number,
						opened_file_info& file,
						const _block_list_t& blocks,
						const _node_pool_t& node_pool,
						const char *buffer,
						size_t size);

				int _get_fid();
				int _regist_to_master();
				int _get_IOnode_socket(int master_number, ssize_t IOnode_id, const std::string& ip);

				static inline int _fd_to_fid(int fd);
				static inline int _fid_to_fd(int fid);
				int _update_fstat_to_server(opened_file_info& file);
				int _get_local_attr(const char*path, struct stat *file_stat);
				file_meta* _create_new_file(Common::extended_IO_task* new_task, int master_number);
				int _close_local_opened_file(const char* path);
				int _get_master_socket_from_path(const std::string& path)const;
				int _get_master_socket_from_master_number(int master_number)const;
				int _get_master_number_from_path(const std::string& path)const;
				int _get_master_socket_from_fd(int fd)const;
				int _get_master_number_from_fd(int fd)const;
				ssize_t _get_IOnode_id(int master_number, ssize_t IOnode_id)const;
				virtual int _report_IOnode_failure(int socket)override final;
				void _parse_master_IOnode_id(ssize_t master_IOnode, int& master_number, ssize_t& IOnode_id);
				ssize_t _find_master_IOnode_id_by_socket(int socket);
				off64_t find_start_point(const _block_list_t& blocks, off64_t start_point, ssize_t update_size);

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
		inline int CBB_client::_fd_to_fid(int fd)
		{
			return fd-INIT_FD;
		}

		inline int CBB_client::_fid_to_fd(int fid)
		{
			return fid+INIT_FD;
		}

		inline int CBB_client::_get_master_socket_from_path(const std::string& path)const
		{
			return master_socket_list.at(_get_master_number_from_path(path));
		}

		inline int CBB_client::_get_master_socket_from_master_number(int master_number)const
		{
			return master_socket_list.at(master_number);
		}

		inline int CBB_client::_get_master_socket_from_fd(int fd)const
		{
			return _file_list.at(fd).file_meta_p->master_socket;
		}

		inline int CBB_client::_get_master_number_from_fd(int fd)const
		{
			return _file_list.at(fd).file_meta_p->master_number;
		}

		inline int CBB_client::_get_master_number_from_path(const std::string& path)const
		{
			static size_t size=master_socket_list.size();
			return file_path_hash(path, size);
		}

		inline ssize_t CBB_client::_get_IOnode_id(int master_number, ssize_t IOnode_id)const
		{
			return master_number*MAX_IONODE+IOnode_id;
		}

		inline void CBB_client::_parse_master_IOnode_id(ssize_t master_IOnode, int& master_number, ssize_t& IOnode_id)
		{
			IOnode_id=master_IOnode%MAX_IONODE;
			master_number=master_IOnode/MAX_IONODE;
		}

	}
}

#endif
