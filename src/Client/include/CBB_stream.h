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

#ifndef CBB_STREAM_H_
#define CBB_STREAM_H_

#include "CBB_client.h"
#include <linux/limits.h>

namespace CBB
{
	namespace Client
	{
		class CBB_stream:
			public CBB_client
		{
			private:
				class stream_info
				{
					public:
						friend class CBB_stream;
						stream_info(bool dirty_flag, bool buffer_flag,
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
						size_t _remaining_buffer_size()const;
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
				static const char* STREAM_USE_BUFFER;
			public:
				CBB_stream();
				~CBB_stream();
				FILE* open_stream(const char* path, const char* mode);
				FILE* open_stream(const char* path, int flag, mode_t mode);
				int flush_stream(FILE* stream);
				int close_stream(FILE* stream);
				void freebuf_stream(FILE* stream);
				void setbuf_stream(FILE* stream, char* buf);
				size_t read_stream(FILE* stream, void* buf, size_t size);
				size_t write_stream(FILE* stream, const void* buf, size_t size);
				off64_t seek_stream(FILE* stream, off64_t offset, int whence);
				off64_t tell_stream(FILE* stream);
				void clearerr_stream(FILE* stream);
				int eof_stream(FILE* stream);
				int error_stream(FILE* stream);
				int fileno_stream(FILE* stream);
				int truncate_stream(FILE* stream, off64_t size);
				bool interpret_stream(void* stream)const;
			private:
				int _init_buffer_for_writing(stream_info_t* file_stream);
				int _flush_stream(stream_info_t* stream);
				int _update_underlying_file_size(FILE* file_stream);

				//const FILE* _get_stream_from_path(const char* path)const;
				//FILE* _get_stream_from_path(const char* path);
			private:
				int _parse_open_mode_flag(const char* path, int& flag, mode_t& open_mode)const;

				stream_pool_t _stream_pool;
		};
		
		inline off64_t CBB_stream::stream_info::
			_get_buf_off_from_file_off(off64_t file_off)const
		{
			return file_off - _cur_file_off() + static_cast<const off64_t>(cur_buf_ptr - buf);
		}

	}
}

#endif
