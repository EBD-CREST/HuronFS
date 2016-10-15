#ifndef CBB_CLIENT_H_

#define CBB_CLIENT_H_

#include <vector>
#include <netinet/in.h>
#include <string>
#include <set>
#include <map>

#include "Client.h"
#include "CBB_const.h"
#include "CBB_basic.h"

namespace CBB
{
	namespace Client
	{
		class CBB_client:
			public Common::Client
		{

			public:
				typedef std::map<int, opened_file_info> _file_list_t; //map fd opened_file_info
				typedef std::vector<bool> _file_t;
				typedef std::vector<block_info> _block_list_t;
				typedef std::map<ssize_t, std::string> _node_pool_t;
				typedef std::set<std::string> dir_t;
				typedef std::vector<SCBB> master_list_t;

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

				ssize_t _read_from_IOnode(SCBB *corresponding_SCBB,
						opened_file_info& file,
						const _block_list_t& blocks,
						const _node_pool_t& node_pool,
						char *buffer,
						size_t size);

				ssize_t _write_to_IOnode(SCBB *corresponding_SCBB,
						opened_file_info& file,
						const _block_list_t& blocks,
						const _node_pool_t& node_pool,
						const char *buffer,
						size_t size);

				int _get_fid();
				int _register_to_master();
				Common::comm_handle_t
					_get_IOnode_handle(SCBB* corresponding_SCBB, 
							   ssize_t IOnode_id,
							   const std::string& ip);

				static inline int _fd_to_fid(int fd);
				static inline int _fid_to_fd(int fid);
				int _update_fstat_to_server(opened_file_info& file);
				int _get_local_attr(const char *path, struct stat *file_stat);
				file_meta* _create_new_file(Common::extended_IO_task* new_task, SCBB* corresponding_SCBB);
				int _close_local_opened_file(const char* path);
				Common::comm_handle_t 
					_get_master_handle_from_path(const std::string& path)const;
				Common::comm_handle_t 
					_get_master_handle_from_master_number(int master_number)const;
				int _get_master_number_from_path(const std::string& path)const;
				Common::comm_handle_t 
					_get_master_handle_from_fd(int fd)const;
				int _get_master_number_from_fd(int fd)const;
				SCBB* _get_scbb_from_master_number(int master_number);
				SCBB* _get_scbb_from_fd(int fd);
				ssize_t _get_IOnode_id(int master_number, ssize_t IOnode_id)const;
				virtual int _report_IOnode_failure(Common::comm_handle_t handle)override final;
				void _parse_master_IOnode_id(ssize_t master_IOnode, int& master_number, ssize_t& IOnode_id);
				SCBB* _find_scbb_by_handle(Common::comm_handle_t handle);
				off64_t find_start_point(const _block_list_t& blocks, off64_t start_point, ssize_t update_size);
				size_t _update_file_size_from_master(Common::extended_IO_task* response, opened_file_info& file);

			private:
				int 			_fid_now;
				_file_list_t 		_file_list;
				_file_t 		_opened_file;
				_path_file_meta_map_t 	_path_file_meta_map;
				struct sockaddr_in 	_client_addr;
				bool 			_initial;

				master_list_t 		master_handle_list;
				std::string		my_uri;
				//IOnode_fd_map_t 	IOnode_fd_map;
		};
		inline int CBB_client::_fd_to_fid(int fd)
		{
			return fd-INIT_FD;
		}

		inline int CBB_client::_fid_to_fd(int fid)
		{
			return fid+INIT_FD;
		}

		inline Common::comm_handle_t CBB_client::
			_get_master_handle_from_path(const std::string& path)const
		{
			return &master_handle_list.at(_get_master_number_from_path(path)).master_handle;
		}

		inline Common::comm_handle_t CBB_client::
			_get_master_handle_from_master_number(int master_number)const
		{
			return &master_handle_list.at(master_number).master_handle;
		}

		inline Common::comm_handle_t CBB_client::
			_get_master_handle_from_fd(int fd)const
		{
			return _file_list.at(fd).file_meta_p->get_master_handle();
		}

		inline SCBB* CBB_client::_get_scbb_from_fd(int fd)
		{
			return _file_list.at(fd).file_meta_p->corresponding_SCBB;
		}

		inline int CBB_client::_get_master_number_from_fd(int fd)const
		{
			return _file_list.at(fd).file_meta_p->get_master_number();
		}

		inline int CBB_client::_get_master_number_from_path(const std::string& path)const
		{
			static size_t size=master_handle_list.size();
			return file_path_hash(path, size);
		}

		inline SCBB* CBB_client::_get_scbb_from_master_number(int master_number)
		{
			return &master_handle_list.at(master_number);
		}

		/*inline ssize_t CBB_client::_get_IOnode_id(int master_number, ssize_t IOnode_id)const
		{
			return master_number*MAX_IONODE+IOnode_id;
		}

		inline void CBB_client::_parse_master_IOnode_id(ssize_t  master_IOnode, 
								int& 	 master_number,
								ssize_t& IOnode_id)
		{
			IOnode_id=master_IOnode%MAX_IONODE;
			master_number=master_IOnode/MAX_IONODE;
		}*/

	}
}

#endif
