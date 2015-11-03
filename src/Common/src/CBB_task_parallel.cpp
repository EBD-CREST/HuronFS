#include <stdlib.h>
#include "CBB_task_parallel.h"

using namespace CBB::Common;

basic_task::basic_task():
	next(NULL)
{}

basic_task::~basic_task()
{}
