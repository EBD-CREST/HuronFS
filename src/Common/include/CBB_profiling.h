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

#include <sys/time.h>
#include "CBB_internal.h"

namespace CBB
{
	namespace Common
	{
#define _RECORD(fp, fmt, args... ) fprintf(fp, "[%s]" fmt, __func__, ##args)

#define record(rec)					\
		do{					\
		(rec).file_name=__FILE__;			\
		(rec).function_name=__func__;		\
		(rec).line_number=__LINE__;		\
		gettimeofday(&(rec).time, nullptr);	\
		}while(0)				\


#define obj_start_recording(obj)		\
		do{			\
			record(obj->st);	\
		}while(0)

#define obj_end_recording(obj)			\
		do{			\
			record(obj->et);	\
			obj->_print_time();	\
		}while(0)
	
#define obj_record_raws(obj)			\
		do{			\
			record(obj->raw);	\
			obj->print_raw_time();\
		}while(0)

#define start_recording()		\
		do{			\
			record(st);	\
		}while(0)

#define end_recording()			\
		do{			\
			record(et);	\
			_print_time();	\
		}while(0)
	
#define record_raws()			\
		do{			\
			record(raw);	\
			print_raw_time();\
		}while(0)

		class time_record
		{
			public:
				time_record()=default;
				~time_record()=default;
				//void record();
				void print();
				ssize_t diff(time_record& et);
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
			private:
				int _open_profile_file(const char* file_name);
				int _close_profile_file();
			public:
				time_record	st;
				time_record	et;
				time_record     raw;
				FILE*		fp; //fp for recording
		};

		inline void time_record::print()
		{
			_LOG("[%s] line %d in file %s\n", this->function_name, this->line_number, this->file_name);
		}
	}
}

#endif
