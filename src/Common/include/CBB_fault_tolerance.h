#ifndef CBB_FAULT_TOLERANCE_H_
#define CBB_FAULT_TOLERANCE_H_

#include "Comm_basic.h"
#include "CBB_IO_task.h"

namespace CBB
{
	namespace Common
	{
		class CBB_fault_tolerance
		{
			public:
				CBB_fault_tolerance()=default;
				~CBB_fault_tolerance()=default;
				virtual int node_failure_handler(extended_IO_task* task)=0;
				virtual int node_failure_handler(comm_handle_t handle)=0;
				virtual int connection_failure_handler(extended_IO_task* task)=0;
		};
	}
}

#endif
