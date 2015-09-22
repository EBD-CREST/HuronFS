#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <pthread.h>
#include <vector>

#include "CBB_queue.h"
#include "CBB_socket.h"

namespace CBB
{
	namespace Common
	{
		struct job
		{
			public:
				job(int socket);
				job();
			private:
				CBB_socket socket;
		};

		struct thread
		{
			public:
				thread(pthread_t thread_hd, int id);
			private:
				pthread_t thread_handler;
				int id;
		};

		class Thread_pool
		{
			public:
				typedef CBB_queue<class job> job_queue_t;
				typedef std::vector<thread> worker_pool_t;

				Thread_pool(int num_threads)throw(std::runtime_error);
				virtual ~Thread_pool();
				void* thread_routine();
				virtual void* do_work(job& new_job)=0;

				void create_thread_pool()throw(std::runtime_error);
				void destory_thread_pool();
				int add_job(job& job);
				void wait_all();
				job detach_job();

			private:
				int num_threads;
				worker_pool_t workers;
				job_queue_t job_queue;
				bool keepAlive;
				pthread_cond_t queue_empty;
				pthread_mutex_t cond_mutex;
				pthread_cond_t queue_not_empty;
				//unthread-safe
		};
	}
}

#endif
