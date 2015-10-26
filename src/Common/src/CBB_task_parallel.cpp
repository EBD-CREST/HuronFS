#include <stdlib.h>
#include "CBB_task_parallel.h"

using namespace CBB::Common;

basic_task::basic_task():
	next(NULL)
{}

basic_task::~basic_task()
{}

basic_IO_task::basic_IO_task():
	basic_task(),
	socket(-1),
	message_size(0),
	basic_message(),
	current_point(0)
{}

basic_IO_task::~basic_IO_task()
{}

extended_IO_task::extended_IO_task():
	basic_IO_task(),
	extended_size(0),
	send_buffer(NULL),
	receive_buffer(NULL)
{}

extended_IO_task::~extended_IO_task()
{}
