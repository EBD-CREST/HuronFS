#include "CBB_profiling.h"
#include <unistd.h>

using namespace CBB;
using namespace CBB::Common;

ssize_t time_record::diff(time_record& et)
{
	struct timeval& et_time=et.time;
	return static_cast<ssize_t>(
			 (et_time.tv_sec-this->time.tv_sec)*1000000
			+(et_time.tv_usec-this->time.tv_usec));
}

void CBB_profiling::_print_time()
{
	//_LOG("start time %ld %ld, end time %ld %ld\n", st.time.tv_sec, st.time.tv_usec, et.time.tv_sec, et.time.tv_usec);
#ifdef PROFILING
	_DEBUG("record\n");
	_RECORD(this->fp, "from %s %d to %s %d difference %ld us\n",
			st.function_name, st.line_number,
			et.function_name, et.line_number, st.diff(et));
	flush_record();
#endif
}

void CBB_profiling::print_raw_time()
{
#ifdef PROFILING
	_RECORD(this->fp, "at %s %d %ld %ld\n",
			raw.function_name, 
			raw.line_number, 
			raw.time.tv_sec, 
			raw.time.tv_usec);
	flush_record();
#endif
}

void CBB_profiling::flush_record()
{
	fflush(this->fp);
}

int CBB_profiling::_open_profile_file(const char* file_name)
{
#ifdef PROFILING
	_close_profile_file();
	if( nullptr == (fp=fopen(file_name, "w")))
	{
		perror("fopen");
		return FAILURE;
	}
#endif
	return SUCCESS;
}

int CBB_profiling::_close_profile_file()
{
	if( nullptr != fp)
	{
		fclose(fp);
	}
	return SUCCESS;
}

CBB_profiling::CBB_profiling():
	st(),
	et(),
	fp(nullptr)
{
#ifdef PROFILING
	char filename[100];
	sprintf(filename, "profile-%d", getpid());
	_open_profile_file(filename);
#endif
}

CBB_profiling::~CBB_profiling()
{
#ifdef PROFILING
	_close_profile_file();
#endif
}
