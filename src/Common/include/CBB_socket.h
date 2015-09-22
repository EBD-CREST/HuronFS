#ifndef CBB_SOCKET_H_
#define CBB_SOCKET_H_

#include <pthread.h>
#include "CBB_mutex_locker.h"

namespace CBB
{
	namespace Common
	{
		class CBB_socket:public CBB_mutex_locker
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
