#ifndef CBB_SOCKET_H_
#define CBB_SOCKET_H_

#include <pthread.h>
#include "CBB_mutex_lock.h"

namespace CBB
{
	namespace Common
	{
		class CBB_socket:public CBB_mutex_lock
		{
			private:
				int socket;
			public:
				CBB_socket();
				CBB_socket(int socket);
				int init(int socket);
				int get();
		};
	}
}

#endif
