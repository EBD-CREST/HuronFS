#ifndef CBB_REQUEST_HANDLER_H_
#define CBB_REQUEST_HANDLER_H_

#include <pthread.h>
#include "CBB_task_parallel.h"

namespace CBB
{
	namespace Common
	{

		class CBB_request_handler
		{
			public:
				CBB_request_handler(task_parallel_queue<IO_task>* input_queue, task_parallel_queue<IO_task>* output_queue);
				CBB_request_handler();
				virtual ~CBB_request_handler();
				virtual int _parse_request(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)=0; 
				int start_handler();
				int stop_handler();
				static void* handle_routine(void *arg);
				void set_queue(task_parallel_queue<IO_task>* input_queue, task_parallel_queue<IO_task>* output_queue);
				//void set_init_barrier(pthread_barrier_t* init_barrier);

			private:
				bool thread_started;
				pthread_t handler_thread;
				//pthread_barrier_t* init_barrier;
				int keepAlive;
				task_parallel_queue<IO_task>* input_queue;
				task_parallel_queue<IO_task>* output_queue;

		};

		//inline void CBB_request_handler::set_init_barrier(pthread_barrier_t* init_barrier)
		//{
		//	this->init_barrier=init_barrier;
		//}
	}
}

#endif
