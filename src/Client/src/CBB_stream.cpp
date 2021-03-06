/*
 * Copyright (c) 2017, Tokyo Institute of Technology
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
 * All rights reserved. 
 * 
 * This file is part of HuronFS.
 * 
 * Please also read the file "LICENSE" included in this package for 
 * Our Notice and GNU Lesser General Public License.
 * 
 * This program is free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License (as published by the Free Software 
 * Foundation) version 2.1 dated February 1999. 
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "CBB_stream.h"
#include "CBB_internal.h"

using namespace CBB::Common;
using namespace CBB::Client;

bool USE_BUFFER=false;
const char* CBB_stream::STREAM_USE_BUFFER="HUFS_STREAM_USE_BUFFER";

CBB_stream::stream_info::
stream_info(	bool 	dirty_flag,
		bool 	buffer_flag,
		int	fd,
		size_t 	file_size,
		off64_t buf_file_off,
		int 	open_flag,
		mode_t 	open_mode,
		allocator& mem_allocator):
	dirty_flag(dirty_flag),
	buffer_flag(buffer_flag),
	fd(fd),
	err(NO_ERROR),
	open_flag(open_flag),
	open_mode(open_mode),
	buf(nullptr),
	cur_buf_ptr(nullptr),
	buffer_size(STREAM_BUFFER_SIZE),
	buffered_data_size(0),
	file_size(file_size),
	buf_file_off(buf_file_off),
	mem_allocator(mem_allocator),
	_elem(nullptr)
{
	if(buffer_flag)
	{
		_elem=mem_allocator.allocate();
		buf=static_cast<char*>(_elem->get_memory());
		cur_buf_ptr=buf;
	}
}

CBB_stream::stream_info::
~stream_info()
{
	free_buffer();
}

CBB_stream::CBB_stream():
	CBB_client(),
	_stream_pool(),
	buffer_pool()
{
	const char* use_buffer=nullptr;
	if(nullptr == (use_buffer=getenv(STREAM_USE_BUFFER)))
	{
		use_buffer="true";
	}

	if(0 == strcmp(use_buffer, "true"))
	{
		USE_BUFFER=true;
	}
	else
	{
		USE_BUFFER=false;
	}
	buffer_pool.setup(STREAM_BUFFER_SIZE,
		MAX_FILE_NUMBER, CLIENT_PRE_ALLOCATE_MEMORY_COUNT);
}

CBB_stream::~CBB_stream()
{
	for(stream_pool_t::iterator it=_stream_pool.begin();
			it!=_stream_pool.end();++it)
	{
		flush_stream(reinterpret_cast<FILE*>(it->first));
		delete it->first;
	}
}

bool CBB_stream::
interpret_stream(void* stream)const
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

int CBB_stream::
_parse_open_mode_flag(	const char* mode,
			int& 	    flags,
			mode_t&     open_mode)const
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

FILE* CBB_stream::
open_stream(const char* path, const char* mode)
{
	int flag=0;
	mode_t open_mode=0;
	//pase mode
	//
	_parse_open_mode_flag(mode, flag, open_mode);
	int fd=CBB_client::open(path, flag, open_mode);
	if(-1 == fd)
	{
		return nullptr;
	}
	else
	{
		size_t file_size=_get_file_size(fd);
		stream_info* new_stream=new stream_info(CLEAN, 
				USE_BUFFER, fd, file_size,
				0, flag, open_mode, buffer_pool);
		_stream_pool.insert(std::make_pair(new_stream, fd));
		return reinterpret_cast<FILE*>(new_stream);
	}
}

FILE* CBB_stream::
open_stream(const char* path, int flag, mode_t mode)
{
	int fd=CBB_client::open(path, flag, mode);

	if(-1 == fd)
	{
		return nullptr;
	}
	else
	{
		size_t file_size=_get_file_size(fd);
		stream_info* new_stream=new stream_info(CLEAN,
				USE_BUFFER, fd, 
				file_size, 0, flag, mode, buffer_pool);
		_stream_pool.insert(std::make_pair(new_stream, fd));
		return reinterpret_cast<FILE*>(new_stream);
	}
}

int CBB_stream::
close_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);

	flush_stream(file_stream);
	_DEBUG("close stream, fd=%d\n", stream->fd);
	CBB_client::close(stream->fd);
	_stream_pool.erase(stream);
	delete stream;
	return SUCCESS;
}

int CBB_stream::
flush_stream(FILE *file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	//forward
	return _flush_stream(stream);
}

int CBB_stream::
_flush_stream(stream_info_t* stream)
{
	_DEBUG("current offset=%ld\n", stream->_cur_file_off());

	if(stream->buffer_flag && DIRTY == stream->dirty_flag)
	{
		_update_file_size(stream->fd, stream->file_size);
		CBB_client::lseek(stream->fd, stream->buf_file_off, SEEK_SET);
		CBB_client::write(stream->fd, stream->buf, stream->buffered_data_size);
		stream->dirty_flag=CLEAN;
		CBB_client::lseek(stream->fd, stream->buffered_data_size, SEEK_CUR);
	}
	return SUCCESS;
}

void CBB_stream::
freebuf_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);

	if(stream->buffer_flag)
	{
		flush_stream(file_stream);
		stream->buffer_flag=false;
		stream->free_buffer();
	}
}

void CBB_stream::
setbuf_stream(FILE* file_stream, char* buf)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);

	if(!stream->buffer_flag)
	{
		stream->buf=buf;
		stream->buffer_flag=true;
		stream->cur_buf_ptr=buf;
	}
}

void CBB_stream::stream_info::
_update_meta_for_rebuf(bool new_dirty_flag, size_t rebuffered_data_size)
{
	buf_file_off=buf_file_off+buffered_data_size;

	dirty_flag=new_dirty_flag;
	buffered_data_size=rebuffered_data_size;
	if(buf_file_off+buffered_data_size > file_size)
	{
		file_size=buf_file_off+buffered_data_size;
	}
	if(_cur_buf_off() >= static_cast<ssize_t>(buffer_size))
	{
		cur_buf_ptr=buf;
	}
}


size_t CBB_stream::stream_info::
_update_cur_buf_ptr(CBB_stream& stream, size_t update_size)
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
	return _cur_buf_off() > static_cast<ssize_t>(buffered_data_size)?
		buffered_data_size:buffered_data_size-_cur_buf_off();
}

inline size_t CBB_stream::stream_info::_remaining_buffer_size()const
{
	return buffer_size - _cur_buf_off();
}

size_t CBB_stream::
read_stream(FILE* file_stream, void* buffer, size_t size)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	//use buffer
	if(stream->buffer_flag)
	{
		size_t remaining_size=stream->_remaining_buffer_data_size();
		size_t buffered_size=MIN(remaining_size, size);
		size_t unbuffered_size=size-buffered_size;
		size_t total_size=0;;
		_DEBUG("remaining_size=%lu, buffered_size=%lu, unbuffered_size=%lu\n",
				remaining_size, buffered_size, unbuffered_size);
		memcpy(buffer, stream->cur_buf_ptr, buffered_size);
		
		stream->_update_cur_buf_ptr(*this, buffered_size);
		total_size+=buffered_size;
		buffer=static_cast<char*>(buffer)+buffered_size;

		while(0 != unbuffered_size)
		{
			//flush data
			flush_stream(file_stream);
			//read from CBB
			size_t ret=CBB_client::read(stream->fd,
					stream->buf, stream->buffer_size);

			//updata meta data
			if(0 == ret)
			{
				return total_size;
			}
			stream->_update_meta_for_rebuf(CLEAN, ret);
			size_t current_IO_size=MIN(unbuffered_size, 
					stream->_remaining_buffer_data_size());
			memcpy(buffer, stream->cur_buf_ptr, current_IO_size);
			stream->_update_cur_buf_ptr(*this, current_IO_size);
			buffer=static_cast<char*>(buffer)+current_IO_size;
			unbuffered_size-=current_IO_size;

			total_size+=current_IO_size;
		}
		return total_size;
	}
	else
	{
		return CBB_client::read(stream->fd, buffer, size); 
	}
}

int CBB_stream::
update_underlying_file_size(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);

	CBB_client::_update_file_size(stream->fd, stream->file_size);
	return SUCCESS;
}

size_t CBB_stream::
stream_info::_write_meta_update(size_t write_size)
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

size_t CBB_stream::
write_stream(FILE* file_stream, const void* buffer, size_t size)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);

	if(!(stream->open_flag & O_WRONLY || stream->open_flag & O_RDWR))
	{
		stream->err=EBADF;
		return 0;
	}

	//use buffer
	if(stream->buffer_flag)
	{
		size_t remaining_size=stream->_remaining_buffer_size();
		size_t bufferable_size=MIN(remaining_size, size);
		size_t unbufferable_size=size-bufferable_size;
		size_t total_size=0;;

		_init_buffer_for_writing(stream);
		_DEBUG("remaining_size=%lu, bufferable_size=%lu, unbufferable_size=%lu\n",
				remaining_size, bufferable_size, unbufferable_size);
		memcpy(stream->cur_buf_ptr, buffer, bufferable_size);
		stream->_write_meta_update(bufferable_size);
		total_size+=bufferable_size;
		stream->dirty_flag=DIRTY;
		buffer=static_cast<const char*>(buffer)+bufferable_size;

		while(0 != unbufferable_size)
		{
			flush_stream(file_stream);

			stream->_update_meta_for_rebuf(DIRTY, 0);
			size_t current_IO_size=MIN(unbufferable_size, stream->_remaining_buffer_size());
			memcpy(stream->buf, buffer, current_IO_size);
			stream->_write_meta_update(current_IO_size);
			buffer=static_cast<const char*>(buffer)+current_IO_size;
			unbufferable_size-=current_IO_size;

			total_size+=current_IO_size;
		}
		return total_size;
	}
	else
	{
		return CBB_client::write(stream->fd, buffer, size); 
	}
}

off64_t CBB_stream::
seek_stream(FILE* file_stream, off64_t offset, int whence)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	off64_t new_pos=0;
	if(stream->buffer_flag)
	{
		_DEBUG("old off=%ld\n", stream->_cur_file_off());
		switch(whence)
		{
			case SEEK_SET:new_pos=offset;break;
			case SEEK_CUR:new_pos=stream->_cur_file_off()+offset;break;
			case SEEK_END:new_pos=static_cast<off64_t>(_get_file_size(stream->fd))+offset;break;
		}

		if( new_pos > static_cast<off64_t>(stream->buf_file_off+stream->buffered_data_size)|| new_pos < stream->buf_file_off)
		{
			_DEBUG("flush stream, new_pos=%ld, current buffered data offset=%ld, current buffer offset =%ld\n",
					new_pos,stream->buf_file_off+stream->buffered_data_size, stream->buf_file_off);
			flush_stream(file_stream);
			off64_t new_block_start_point=get_block_start_point(new_pos);
			stream->buf_file_off=new_block_start_point;
			stream->buffered_data_size=0;
			stream->cur_buf_ptr=stream->buf+new_pos-new_block_start_point;
			CBB_client::lseek(stream->fd, new_block_start_point, SEEK_SET);
		}
		else
		{
			stream->cur_buf_ptr = stream->buf + stream->_get_buf_off_from_file_off(new_pos);
		}
		_DEBUG("new off=%ld\n", stream->_cur_file_off());
	}
	else
	{
		new_pos=CBB_client::lseek(stream->fd, offset, whence);
	}
	return new_pos;
}

off64_t CBB_stream::
tell_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	if(stream->buffer_flag)
	{
		return stream->buf_file_off+(stream->cur_buf_ptr-stream->buf);
	}
	else
	{
		return CBB_client::tell(stream->fd);
	}
}

void CBB_stream::
clearerr_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	stream->err=NO_ERROR;
}

int CBB_stream::
eof_stream(FILE* file_stream)
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

int CBB_stream::
error_stream(FILE* file_stream)
{
	stream_info_t* stream=reinterpret_cast<stream_info_t*>(file_stream);
	return stream->err;
}

int CBB_stream::
fileno_stream(FILE* file_stream)
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
		return nullptr;
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
		return nullptr;
	}
}*/

int CBB_stream::
truncate_stream(FILE* file_stream, off64_t size)
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
			if(static_cast<off64_t>(stream->buffered_data_size)>size-stream->buf_file_off)
			{
				stream->buffered_data_size=size-stream->buf_file_off;
				stream->cur_buf_ptr=stream->buf+stream->buffered_data_size;
			}
		}
	}
	return CBB_client::ftruncate(stream->fd, size);
}

int CBB_stream::_init_buffer_for_writing(stream_info_t* stream)
{
	//if there is data remaining in file (current offset < total file size)
	//and we will write beyond end of current buffered space
	//then we need to retrive data from remote
	if(stream->_cur_buf_off() > 
		static_cast<ssize_t>(stream->buffered_data_size) &&
		stream->buf_file_off < static_cast<ssize_t>(stream->file_size))
	{
		_DEBUG("fill buffer before writing buffer offset=%ld, buffered_data_size=%ld\n", stream->_cur_buf_off(), stream->buffered_data_size);
		_DEBUG("file offset=%ld, file size=%ld\n", stream->buf_file_off, stream->file_size);
		_flush_stream(stream);

		return CBB_client::read(stream->fd, stream->buf, stream->buffer_size);
	}
	else
	{
		_DEBUG("no need to fill buffer\n");
		return SUCCESS;
	}
}
