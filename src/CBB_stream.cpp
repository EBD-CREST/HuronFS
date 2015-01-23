#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "include/CBB_stream.h"

CBB_stream::stream_info::stream_info(bool dirty_flag,
		bool buffer_flag,
		int fd,
		off64_t cur_file_off,
		int open_flag,
		mode_t open_mode):
	eof(false),
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
	file_size(0),
	cur_file_off(cur_file_off)
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
			case 'r' : flags = (flags & ~3) | O_RDONLY;
				   break;
			case 'w' : flags = (flags & ~3) | O_WRONLY | O_CREAT | O_TRUNC;
				   break;
			case 'a' : flags |= O_APPEND;
				   break;
			case '+' : flags = (flags & ~3) | O_RDWR;
				   break;
		}

	open_mode = 0666;
	return SUCCESS;
}

FILE* CBB_stream::_open_stream(const char* path, const char* mode)
{
	int flag;
	mode_t open_mode;
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
		stream_info* new_stream=new stream_info(CLEAN, true, fd, 0, flag, open_mode);
		_stream_pool.insert(std::make_pair(new_stream, fd));
		return reinterpret_cast<FILE*>(new_stream);
	}
}

int CBB_stream::_close_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	_flush_stream(file_stream);
	_close(stream->fd);
	_stream_pool.erase(stream);
	return SUCCESS;
}

int CBB_stream::_flush_stream(FILE* file_stream)
{

	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	if(DIRTY == stream->dirty_flag)
	{
		_write(stream->fd, stream->buf, stream->buffered_data_size);
		stream->dirty_flag=CLEAN;
	}
	_lseek(stream->fd, stream->buffered_data_size, SEEK_CUR);
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

void CBB_stream::_update_meta_for_rebuf(stream_info_t* stream, bool dirty_flag, size_t buffered_data_size)
{
	stream->cur_file_off=stream->cur_file_off+stream->buffered_data_size;

	stream->dirty_flag=dirty_flag;
	stream->buffered_data_size=buffered_data_size;
	if(stream->cur_file_off+stream->buffered_data_size > stream->file_size)
	{
		stream->file_size=stream->cur_file_off+stream->buffered_data_size;
	}
}

size_t CBB_stream::_read_stream(FILE* file_stream, void* buffer, size_t size)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	//use buffer
	if(stream->buffer_flag)
	{
		size_t remaining_size=stream->buffered_data_size-(stream->cur_buf_ptr-stream->buf);
		size_t buffered_size=remaining_size>size?size:remaining_size;
		size_t unbuffered_size=size-buffered_size;
		size_t total_size=0;;
		memcpy(buffer, stream->cur_buf_ptr, buffered_size);
		stream->cur_buf_ptr+=buffered_size;
		total_size+=buffered_size;
		buffer=static_cast<char*>(buffer)+buffered_size;

		while(0 != unbuffered_size)
		{
			//flush data
			_flush_stream(file_stream);
			//read from CBB
			size_t ret=_read(stream->fd, stream->buf, stream->buffer_size);

			//updata meta data
			_update_meta_for_rebuf(stream, CLEAN, ret);
			size_t current_IO_size=unbuffered_size > stream->buffered_data_size?stream->buffered_data_size:unbuffered_size;
			memcpy(buffer, stream->buf, current_IO_size);
			stream->cur_buf_ptr=stream->buf+current_IO_size;
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

size_t CBB_stream::_write_stream(FILE* file_stream, const void* buffer, size_t size)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	//use buffer
	
	if(stream->open_flag & O_WRONLY || stream->open_flag & O_RDWR)
	{
		stream->err=EBADF;
		return 0;
	}

	if(stream->buffer_flag)
	{
		size_t remaining_size=stream->buffer_size-(stream->cur_buf_ptr-stream->buf);
		size_t buffered_size=remaining_size>size?size:remaining_size;
		size_t unbuffered_size=size-buffered_size;
		size_t total_size=0;;
		memcpy(stream->cur_buf_ptr, buffer, buffered_size);
		stream->cur_buf_ptr+=buffered_size;
		if(stream->cur_buf_ptr-stream->buf > stream->buffered_data_size)
		{
			stream->buffered_data_size=stream->cur_buf_ptr-stream->buf;
		}
		total_size+=buffered_size;
		stream->dirty_flag=DIRTY;
		buffer=static_cast<const char*>(buffer)+buffered_size;

		while(0 != unbuffered_size)
		{
			_flush_stream(file_stream);

			_update_meta_for_rebuf(stream, DIRTY, stream->buffered_data_size);
			size_t current_IO_size=unbuffered_size>stream->buffered_data_size?stream->buffered_data_size:unbuffered_size;
			memcpy(stream->buf, buffer, current_IO_size);
			stream->cur_buf_ptr=stream->buf+current_IO_size;
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
	off64_t new_pos;
	switch(whence)
	{
		case SEEK_SET:new_pos=offset;break;
		case SEEK_CUR:new_pos+=stream->cur_file_off+stream->cur_buf_ptr-stream->buf+offset;break;
		case SEEK_END:new_pos;break;
	}
	if( new_pos > stream->cur_file_off+stream->cur_buf_ptr-stream->buf || new_pos < stream->cur_file_off)
	{
		_flush_stream(file_stream);
	}
	return new_pos;
}

off64_t CBB_stream::_tell_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	return stream->cur_file_off+(stream->cur_buf_ptr-stream->buf);
}

void CBB_stream::_clearerr_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	stream->err=NO_ERROR;
}

int CBB_stream::_eof_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	return stream->eof;
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
