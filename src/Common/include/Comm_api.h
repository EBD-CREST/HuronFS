#ifndef COMM_API_H_
#define COMM_API_H_

#ifdef TCP

#include "comm_type/tcp.h"

namespace CBB
{
	namespace Common
	{
		typedef CBB_tcp CBB_communication;

	}
}
#else

#include "comm_type/cci.h"

namespace CBB
{
	namespace Common
	{
		typedef CBB_cci CBB_communication;
	}
}

#endif

#endif
