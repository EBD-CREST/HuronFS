#ifndef CBB_FAULT_TOLERANCE_H_
#define CBB_FAULT_TOLERANCE_H_

#include "Comm_basic.h"
namespace CBB
{
	namespace Common
	{
		class CBB_fault_tolerance
		{
			public:
				CBB_fault_tolerance()=default;
				~CBB_fault_tolerance()=default;
				virtual int node_failure_handler(comm_handle_t node_socket)=0;
		};
	}
}

#endif
