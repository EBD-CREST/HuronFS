#ifndef CBB_REQUEST_HANDLER_H_
#define CBB_REQUEST_HANDLER_H_

#include <pthread.h>
#include "CBB_task_parallel.h"
#include "CBB_communication_thread.h"

namespace CBB
{
	namespace Common
	{

		class CBB_request_handler
		{
			public:
				CBB_request_handler(communication_queue_array_t* input_queue,
						communication_queue_array_t* output_queue);
				CBB_request_handler();
				virtual ~CBB_request_handler();
				virtual int _parse_request(extended_IO_task* new_task)=0; 
				int start_handler();
				int stop_handler();
				static void* handle_routine(void *arg);

				void set_queues(communication_queue_array_t* input_queue,
						communication_queue_array_t* output_queue);

			private:
				bool thread_started;
				pthread_t handler_thread;
				int keepAlive;
				communication_queue_array_t* input_queue;
				communication_queue_array_t* output_queue;

		};
	}
}

#endif
