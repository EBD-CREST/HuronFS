#ifndef CBB_REMOTE_TASK_H_
#define CBB_REMOTE_TASK_H_

#include "CBB_task_parallel.h"
#include <pthread.h>

namespace CBB
{
	namespace Common
	{

		class remote_task:public basic_task
		{
			public:
				remote_task();
				virtual ~remote_task();
				void set_mode(int mode);
				int get_mode()const;
				void set_file_stat(void* file_stat);
				void* get_file_stat();
			private:
				int mode;
				void *file_stat;
		};

		class CBB_remote_task
		{
			public:
				CBB_remote_task();
				virtual ~CBB_remote_task();
				virtual int remote_task_handler(remote_task* new_task)=0;
				int start_listening();
				void stop_listening();
				remote_task* add_remote_task(int task_code, void* file);
				static void* thread_fun(void* args);
			private:
				int keepAlive;
				bool thread_started;
				task_parallel_queue<remote_task> remote_task_queue;
				pthread_t remote_task_thread;
				pthread_mutex_t locker;
		};

		inline void remote_task::set_mode(int mode)
		{
			this->mode=mode;
		}

		inline int remote_task::get_mode()const
		{
			return this->mode;
		}

		inline void remote_task::set_file_stat(void* file_stat)
		{
			this->file_stat=file_stat;
		}

		inline void* remote_task::get_file_stat()
		{
			return this->file_stat;
		}
	}
}

#endif
