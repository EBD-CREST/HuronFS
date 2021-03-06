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

#ifndef CBB_PROFILING_H_
#define CBB_PROFILING_H_

#include "CBB_internal.h"

namespace CBB
{
	namespace Common
	{
#define _RECORD(fp, fmt, args... ) fprintf(fp, "[%s]" fmt, __func__, ##args)

#define start_recording(obj)	\
		do{				\
			(obj)->st.record();	\
		}while(0)

#define end_recording(obj, size, mode)		\
		do{				\
			(obj)->et.record();	\
			(obj)->record_size(size, mode);\
		}while(0)
			//(obj)->_print_time();	
	
#define record_raws(obj)			\
		do{				\
			(obj)->raw.record();	\
			(obj)->print_raw_time();\
		}while(0)

		class time_record
		{
			public:
				time_record()=default;
				~time_record()=default;
				//void record();
				void print();
				double diff(time_record& et);
				void record();
				size_t get_sec()const;
				size_t get_usec()const;
			public:
				const char*	file_name;
				const char*	function_name; 
				int 		line_number;
				struct timeval	time;
		};

		class CBB_profiling
		{
			public:
				CBB_profiling();
				virtual ~CBB_profiling();
				//void start_recording();
				//void end_recording();
			public:
				void _print_time();
				void print_raw_time();
				void flush_record();
				void record_size(size_t size, int mode);
				void print_log(	const char* mode, 
					const char* io_path,
					size_t offset,
					size_t length);
				void print_log_debug(	const char* mode, 
					const char* io_path,
					size_t offset,
					size_t length);
			private:
				int _open_profile_file(const char* file_name);
				int _close_profile_file();
				bool is_profiling()const;
				static const char* HUFS_PROFILING;
				static const char* HUFS_PROFILE_PATH;
			public:
				time_record	st;
				time_record	et;
				time_record     raw;
				FILE*		fp; //fp for recording
				bool		profiling_flag;

				double		last_duration;
				double 		total_time;
				size_t		total_read;
				size_t		total_write;
				int 		delay_count;
				size_t		new_size;
		};

		inline void time_record::
			print()
		{
			_LOG("[%s] line %d in file %s\n", this->function_name, this->line_number, this->file_name);
		}

		inline size_t time_record::
			get_sec()const
		{
			return this->time.tv_sec;
		}

		inline size_t time_record::
			get_usec()const
		{
			return this->time.tv_usec;
		}

		inline bool CBB_profiling::
			is_profiling()const
		{
			return profiling_flag;
		}

		inline void CBB_profiling::
			record_size(size_t size, int mode)
		{
			if(READ_FILE == mode || RMA_READ == mode)
			{
				total_read += size;
			}
			else
			{
				total_write += size;
			}
			new_size += size;
		}

		inline void CBB_profiling::
			print_log(	const char* mode, 
					const char* io_path,
					size_t offset,
					size_t length)
		{
			if(is_profiling())
			{
				_RECORD(fp, "%ld.%ld\t%f\t0\t%s\t%s\t%lu\t%lu\n", et.get_sec(), et.get_usec(), 
						st.diff(et), mode, io_path, offset, length);
			}
		}

		inline void CBB_profiling::
			print_log_debug(const char* mode, 
					const char* io_path,
					size_t offset,
					size_t length)
		{
			if(is_profiling())
			{
				_LOG("%ld.%ld\t%f\t0\t%s\t%s\t%lu\t%lu\n", et.get_sec(), et.get_usec(), 
						st.diff(et), mode, io_path, offset, length);
			}
		}
	}
}

#endif
