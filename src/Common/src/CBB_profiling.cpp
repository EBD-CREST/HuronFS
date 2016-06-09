#include "CBB_profiling.h"

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
	_LOG("from %s %d to %s %d difference %ld us\n", st.function_name, st.line_number
			,et.function_name, et.line_number, st.diff(et));
}
