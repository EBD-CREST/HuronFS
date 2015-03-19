#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "CBB_stream.h"
#include "CBB_internal.h"

CBB_stream::stream_info::stream_info(bool dirty_flag,
		bool buffer_flag,
		int fd,
		size_t file_size,
		off64_t buf_file_off,
		int open_flag,
		mode_t open_mode):
	dirty_flag(dirty_flag),
	buffer_flag(buffer_flag),
	fd(fd),
	err(NO_ERROR),
	open_flag(open_flag),
	open_mode(open_mode),
	buf(NULL),
	cur_buf_ptr(NULL),
	buffer_size(STREAM_BUFFER_SIZE),
	buffered_data_size(0),
	file_size(file_size),
	buf_file_off(buf_file_off)
{
	if(buffer_flag)
	{
		buf=new char[STREAM_BUFFER_SIZE];
		cur_buf_ptr=buf;
	}
}

CBB_stream::stream_info::~stream_info()
{
	delete[] buf;
}

CBB_stream::CBB_stream():
	CBB(),
	_stream_pool()
{}

CBB_stream::~CBB_stream()
{
	for(stream_pool_t::iterator it=_stream_pool.begin();
			it!=_stream_pool.end();++it)
	{
		_flush_stream(reinterpret_cast<FILE*>(it->first));
		delete it->first;
	}
}

bool CBB_stream::_interpret_stream(void* stream)const
{
	stream_info_t* stream_info_ptr=static_cast<stream_info_t*>(stream);
	if(_stream_pool.end() == _stream_pool.find(stream_info_ptr))
	{
		return false;
	}
	else
	{
		return true;
	}
}

int CBB_stream::_parse_open_mode_flag(const char* mode, int& flags, mode_t& open_mode)const
{
	while (*mode)
		switch (*mode++) {
			case 'r' : flags = flags | O_RDONLY;
				   break;
			case 'w' : flags = flags | O_WRONLY | O_CREAT | O_TRUNC;
				   break;
			case 'a' : flags |= O_APPEND;
				   break;
			case '+' : flags = flags | O_RDWR;
				   break;
		}

	open_mode = 0666;
	return SUCCESS;
}

FILE* CBB_stream::_open_stream(const char* path, const char* mode)
{
	int flag=0;
	mode_t open_mode=0;
	//pase mode
	//
	_parse_open_mode_flag(mode, flag, open_mode);
	int fd=_open(path, flag, open_mode);
	if(-1 == fd)
	{
		return NULL;
	}
	else
	{
		size_t file_size=_get_file_size(fd);
		stream_info* new_stream=new stream_info(CLEAN, true, fd, file_size, 0, flag, open_mode);
		_stream_pool.insert(std::make_pair(new_stream, fd));
		return reinterpret_cast<FILE*>(new_stream);
	}
}

FILE* CBB_stream::_open_stream(const char* path, int flag, mode_t mode)
{
	int fd=_open(path, flag, mode);
	if(-1 == fd)
	{
		return NULL;
	}
	else
	{
		size_t file_size=_get_file_size(fd);
		stream_info* new_stream=new stream_info(CLEAN, true, fd, file_size, 0, flag, mode);
		_stream_pool.insert(std::make_pair(new_stream, fd));
		return reinterpret_cast<FILE*>(new_stream);
	}
}

int CBB_stream::_close_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	_flush_stream(file_stream);
	_DEBUG("close stream, fd=%d\n", stream->fd);
	_close(stream->fd);
	_stream_pool.erase(stream);
	stream->~stream_info();
	return SUCCESS;
}

int CBB_stream::_flush_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	_DEBUG("current offset=%ld\n", stream->_cur_file_off());
	if(DIRTY == stream->dirty_flag)
	{
		_update_file_size(stream->fd, stream->file_size);
		_lseek(stream->fd, stream->buf_file_off, SEEK_SET);
		_write(stream->fd, stream->buf, stream->buffered_data_size);
		stream->dirty_flag=CLEAN;
		_lseek(stream->fd, stream->buffered_data_size, SEEK_CUR);
	}
	return SUCCESS;
}

void CBB_stream::_freebuf_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	if(stream->buffer_flag)
	{
		_flush_stream(file_stream);
		stream->cur_buf_ptr=NULL;
		stream->buffered_data_size=0;
		delete[] stream->buf;
		stream->buffer_flag=false;
	}
}

void CBB_stream::_setbuf_stream(FILE* file_stream, char* buf)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	if(!stream->buffer_flag)
	{
		stream->buf=buf;
		stream->buffer_flag=true;
		stream->cur_buf_ptr=buf;
	}
}

void CBB_stream::stream_info::_update_meta_for_rebuf(bool new_dirty_flag, size_t rebuffered_data_size)
{
	buf_file_off=buf_file_off+buffered_data_size;

	dirty_flag=new_dirty_flag;
	buffered_data_size=rebuffered_data_size;
	if(buf_file_off+buffered_data_size > file_size)
	{
		file_size=buf_file_off+buffered_data_size;
	}
	cur_buf_ptr=buf;
}


size_t CBB_stream::stream_info::_update_cur_buf_ptr(CBB_stream& stream, size_t update_size)
{
	_update_file_size(stream);
	if(file_size<  update_size + _cur_file_off())
	{
		cur_buf_ptr += file_size - _cur_file_off();
		return file_size - _cur_file_off();
	}
	else
	{
		cur_buf_ptr+=update_size;
		return update_size;
	}
}

inline off64_t CBB_stream::stream_info::_cur_buf_off()const
{
	return cur_buf_ptr-buf;
}

inline off64_t CBB_stream::stream_info::_cur_file_off()const
{
	return buf_file_off+_cur_buf_off();
}

inline void CBB_stream::stream_info::_update_file_size(CBB_stream& stream)
{
	if(file_size < stream._get_file_size(fd))
	{
		file_size = stream._get_file_size(fd);
	}
}

inline size_t CBB_stream::stream_info::_remaining_buffer_data_size()const
{
	return buffered_data_size-_cur_buf_off();
}

inline size_t CBB_stream::stream_info::_remaining_buffer_size()const
{
	return buffer_size - _cur_buf_off();
}

size_t CBB_stream::_read_stream(FILE* file_stream, void* buffer, size_t size)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	//use buffer
	if(stream->buffer_flag)
	{
		size_t remaining_size=stream->_remaining_buffer_data_size();
		size_t buffered_size=MIN(remaining_size, size);
		size_t unbuffered_size=size-buffered_size;
		size_t total_size=0;;
		_DEBUG("remaining_size=%lu, buffered_size=%lu, unbuffered_size=%lu\n", remaining_size, buffered_size, unbuffered_size);
		memcpy(buffer, stream->cur_buf_ptr, buffered_size);
		
		stream->_update_cur_buf_ptr(*this, buffered_size);
		total_size+=buffered_size;
		buffer=static_cast<char*>(buffer)+buffered_size;

		while(0 != unbuffered_size)
		{
			//flush data
			_flush_stream(file_stream);
			//read from CBB
			size_t ret=_read(stream->fd, stream->buf, stream->buffer_size);

			//updata meta data
			if(0 == ret)
			{
				return total_size;
			}
			/*if(ret != stream->buffer_size)
			{
				return total_size;
			}*/
			stream->_update_meta_for_rebuf(CLEAN, ret);
			size_t current_IO_size=MIN(unbuffered_size, stream->buffered_data_size);
			memcpy(buffer, stream->buf, current_IO_size);
			stream->_update_cur_buf_ptr(*this, current_IO_size);
			buffer=static_cast<char*>(buffer)+current_IO_size;
			unbuffered_size-=current_IO_size;

			total_size+=current_IO_size;
		}
		return total_size;
	}
	else
	{
		return _read(stream->fd, buffer, size); 
	}
}

int CBB_stream::_update_underlying_file_size(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	CBB::_update_file_size(stream->fd, stream->file_size);
	return SUCCESS;
}

size_t CBB_stream::stream_info::_write_meta_update(size_t write_size)
{

	cur_buf_ptr+=write_size;
	if(_cur_buf_off() > static_cast<off64_t>(buffered_data_size))
	{
		buffered_data_size=_cur_buf_off();
	}
	if((size_t)_cur_file_off() > file_size)
	{
		file_size = _cur_file_off();
	}
	return write_size;
}

size_t CBB_stream::_write_stream(FILE* file_stream, const void* buffer, size_t size)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	//use buffer

	if(!(stream->open_flag & O_WRONLY || stream->open_flag & O_RDWR))
	{
		stream->err=EBADF;
		return 0;
	}

	if(stream->buffer_flag)
	{
		size_t remaining_size=stream->_remaining_buffer_size();
		size_t buffered_size=MIN(remaining_size, size);
		size_t unbuffered_size=size-buffered_size;
		size_t total_size=0;;
		_DEBUG("remaining_size=%lu, buffered_size=%lu, unbuffered_size=%lu\n", remaining_size, buffered_size, unbuffered_size);
		memcpy(stream->cur_buf_ptr, buffer, buffered_size);
		stream->_write_meta_update(buffered_size);
		total_size+=buffered_size;
		stream->dirty_flag=DIRTY;
		buffer=static_cast<const char*>(buffer)+buffered_size;

		while(0 != unbuffered_size)
		{
			_flush_stream(file_stream);

			stream->_update_meta_for_rebuf(DIRTY, stream->buffered_data_size);
			/*if(ret != stream->buffered_data_size)
			{
				return total_size;
			}*/
			size_t current_IO_size=MIN(unbuffered_size, stream->buffer_size);
			memcpy(stream->buf, buffer, current_IO_size);
			stream->_write_meta_update(current_IO_size);
			buffer=static_cast<const char*>(buffer)+current_IO_size;
			unbuffered_size-=current_IO_size;

			total_size+=current_IO_size;
		}
		return total_size;
	}
	else
	{
		return _write(stream->fd, buffer, size); 
	}
}

off64_t CBB_stream::_seek_stream(FILE* file_stream, off64_t offset, int whence)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	off64_t new_pos=0;
	_DEBUG("old off=%ld\n", stream->_cur_file_off());
	switch(whence)
	{
		case SEEK_SET:new_pos=offset;break;
		case SEEK_CUR:new_pos=stream->_cur_file_off()+offset;break;
		case SEEK_END:new_pos=static_cast<off64_t>(_get_file_size(stream->fd))+offset;break;
	}
	if( new_pos > stream->buf_file_off+stream->buffered_data_size|| new_pos < stream->buf_file_off)
	{
		_DEBUG("flush stream, new_pos=%ld, current buffered data offset=%ld, current buffer offset =%d\n",
				new_pos,stream->buf_file_off+stream->buffered_data_size, stream->buf_file_off);
		_flush_stream(file_stream);
		stream->cur_buf_ptr=stream->buf;
		stream->buf_file_off=new_pos;
		stream->buffered_data_size=0;
		_lseek(stream->fd, new_pos, SEEK_SET);
	}
	else
	{
		stream->cur_buf_ptr = stream->buf + stream->_get_buf_off_from_file_off(new_pos);
	}
	_DEBUG("new off=%ld\n", stream->_cur_file_off());
	return new_pos;
}

inline off64_t CBB_stream::stream_info::_get_buf_off_from_file_off(off64_t file_off)const
{
	return file_off - _cur_file_off() + static_cast<const off64_t>(cur_buf_ptr - buf);
}

off64_t CBB_stream::_tell_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	return stream->buf_file_off+(stream->cur_buf_ptr-stream->buf);
}

void CBB_stream::_clearerr_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	stream->err=NO_ERROR;
}

int CBB_stream::_eof_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	if(stream->_cur_file_off() < static_cast<off64_t>(stream->file_size))
	{
		return false;
	}
	else
	{
		return true;
	}
}

int CBB_stream::_error_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	return stream->err;
}

int CBB_stream::_fileno_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	return stream->fd;
}

/*FILE* CBB_stream::_get_stream_from_path(const char* path)
{
	try
	{
		stream_info* stream=_path_stream_map.at(std::string(path));
		return reinterpret_cast<FILE*>(stream);
	}
	catch(std::out_of_range &e)
	{
		fprintf(stderr, "open file frist\n");
		return NULL;
	}
}

const FILE* CBB_stream::_get_stream_from_path(const char* path)const
{
	try
	{
		const stream_info* stream=_path_stream_map.at(std::string(path));
		return reinterpret_cast<const FILE*>(stream);
	}
	catch(std::out_of_range &e)
	{
		fprintf(stderr, "open file frist\n");
		return NULL;
	}
}*/

int CBB_stream::_truncate_stream(FILE* file_stream, off64_t size)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	stream->file_size=size;
	if(stream->buffer_flag)
	{
		if(stream->_cur_file_off() > size)
		{
			if(stream->buf_file_off > size)
			{
				stream->buf_file_off=size;
			}
			if(stream->buffered_data_size>size-stream->buf_file_off)
			{
				stream->buffered_data_size=size-stream->buf_file_off;
				stream->cur_buf_ptr=stream->buf+stream->buffered_data_size;
			}
		}
	}
	return _ftruncate(stream->fd, size);
}

