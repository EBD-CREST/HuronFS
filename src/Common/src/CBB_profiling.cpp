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

#include "CBB_profiling.h"
#include <unistd.h>
#include <limits.h>

using namespace CBB;
using namespace CBB::Common;

const char* CBB_profiling::
HUFS_PROFILING="HUFS_PROFILING";

const char* CBB_profiling::
HUFS_PROFILE_PATH="HUFS_PROFILING_PATH";

double time_record::diff(time_record& et)
{
	struct timeval& et_time=et.time;
	return TIME(this->time, et_time);
}

void time_record::
record()
{
	file_name=__FILE__;
	function_name=__func__;
	line_number=__LINE__;	
	gettimeofday(&time, nullptr);
}

void CBB_profiling::_print_time()
{
	//_LOG("start time %ld %ld, end time %ld %ld\n", st.time.tv_sec, st.time.tv_usec, et.time.tv_sec, et.time.tv_usec);
	if(is_profiling())
	{
		_DEBUG("record\n");
		last_duration=st.diff(et);
		total_time += last_duration;

		if( 0 == -- delay_count)
		{
			double total_size = (total_read + total_write)/(double)MB;
			_RECORD(this->fp, "added time %f s, added size %ld, total time %lf s, \
					total read %f MB, total write %f MB, total_size %f MB, throughput %f MB/s\n",
					last_duration, new_size, total_time, total_read/(double)MB, total_write/(double)MB, total_size, total_size/total_time);
			new_size=0;
			delay_count=PRINT_DELAY;
			/*_RECORD(this->fp, "from %s %d to %s %d difference %ld us\n",
			  st.function_name, st.line_number,
			  et.function_name, et.line_number, new_time);*/
		}
	}
}

CBB_profiling::
~CBB_profiling()
{
	if(is_profiling())
	{
		_close_profile_file();

		if(PRINT_DELAY != delay_count)
		{
			double new_time=st.diff(et);
			total_time += new_time;
			double total_size = (total_read + total_write)/(double)MB;

			_RECORD(this->fp, "added time %f s, added size %ld, total time %lf s, \
					total read %f MB, total write %f MB, total_size %f MB, throughput %f MB/s\n",
					new_time, new_size, total_time, total_read/(double)MB, total_write/(double)MB, total_size, total_size/total_time);
		}
	}
}


void CBB_profiling::print_raw_time()
{
	if(is_profiling())
	{
		_RECORD(this->fp, "at %s %d %ld %ld\n",
				raw.function_name, 
				raw.line_number, 
				raw.time.tv_sec, 
				raw.time.tv_usec);
	}
}

void CBB_profiling::
flush_record()
{
	if(is_profiling())
	{
		fflush(this->fp);
	}
}

int CBB_profiling::
_open_profile_file(const char* file_name)
{
	if(is_profiling())
	{
		_close_profile_file();
		if( nullptr == (fp=fopen(file_name, "w")))
		{
			perror("fopen");
			return FAILURE;
		}
	}
	return SUCCESS;
}

int CBB_profiling::
_close_profile_file()
{
	if( is_profiling() &&
		nullptr != fp)
	{
		fclose(fp);
	}
	return SUCCESS;
}

CBB_profiling::CBB_profiling():
	st(),
	et(),
	raw(),
	fp(nullptr),
	profiling_flag(false),
	last_duration(0),
	total_time(0.0),
	total_read(0),
	total_write(0),
	delay_count(PRINT_DELAY)
{
	const char* hufs_profiling=nullptr;
	const char* hufs_profile_path=".";
	const char* user_profile_path=nullptr;
	if(nullptr != (hufs_profiling = 
			getenv(HUFS_PROFILING)))
	{
		if(0 == strcmp("true", hufs_profiling))
		{
			profiling_flag=true;
		}
		else
		{
			if(0 == strcmp("false", hufs_profiling))
			{
				profiling_flag=false;
			}
			else
			{
				_LOG("please set %s to true or false, use false\n",\
					HUFS_PROFILING);
			}
		}
	}

	if(nullptr != (user_profile_path =
		getenv(HUFS_PROFILE_PATH)))
	{
		hufs_profile_path=user_profile_path;
	}

	if(is_profiling())
	{
		_LOG("profiling\n");
		char filename[PATH_MAX];
		//may have the same pid
		sprintf(filename, "%s/profile-%d", hufs_profile_path, getpid());
		_open_profile_file(filename);
	}
}
