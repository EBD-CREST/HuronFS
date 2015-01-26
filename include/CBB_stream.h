#ifndef CBB_STREAM_H_
#define CBB_STREAM_H_

#include "include/CBB.h"

class CBB_stream:public CBB
{
private:
	class stream_info
	{
	public:
		friend class CBB_stream;
		stream_info(bool dirty_flag,
				bool buffer_flag,
				int fd,
				size_t file_size,
				off64_t cur_file_ptr,
				int open_flag,
				mode_t open_mode);
		~stream_info();
		size_t _update_cur_buf_ptr(CBB_stream& stream, size_t size);
		off64_t _cur_buf_off()const;
		off64_t _cur_file_off()const;
		void _update_file_size(CBB_stream& stream);
		size_t _remaining_buffer_data_size()const;
		size_t _write_meta_update(size_t write_size);
		void _update_meta_for_rebuf(bool dirty_flag, size_t update_size);
		off64_t _get_buf_off_from_file_off(off64_t file_off)const;
	private:
		bool dirty_flag;
		bool buffer_flag;
		int fd;
		int err;
		int open_flag;
		mode_t open_mode;
		char* buf;
		char* cur_buf_ptr;
		size_t buffer_size;
		size_t buffered_data_size;
		size_t file_size;
		//buf start point file offset
		off64_t buf_file_off;
	private:
		stream_info(const stream_info&);
	};
private:

	typedef stream_info stream_info_t;
	typedef std::map<stream_info_t*, int> stream_pool_t;
public:
	CBB_stream();
	~CBB_stream();
	bool _interpret_stream(void* stream)const;
	FILE* _open_stream(const char* path, const char* mode);
	int _flush_stream(FILE* stream);
	int _close_stream(FILE* stream);
	void _freebuf_stream(FILE* stream);
	void _setbuf_stream(FILE* stream, char* buf);
	size_t _read_stream(FILE* stream, void* buf, size_t size);
	size_t _write_stream(FILE* stream, const void* buf, size_t size);
	off64_t _seek_stream(FILE* stream, off64_t offset, int whence);
	off64_t _tell_stream(FILE* stream);
	void _clearerr_stream(FILE* stream);
	int _eof_stream(FILE* stream);
	int _error_stream(FILE* stream);
	int _fileno_stream(FILE* stream);
private:
	stream_pool_t _stream_pool;
	int _parse_open_mode_flag(const char* path, int& flag, mode_t& open_mode)const;
};

#endif
