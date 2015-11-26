#ifndef CBB_TASK_PARALLEL_H_
#define CBB_TASK_PARALLEL_H_

#include <pthread.h>

#include "CBB_mutex_locker.h"
#include "CBB_const.h"
#include "Communication.h"
#include "string.h"

namespace CBB
{
	namespace Common
	{
		template<typename T> inline T atomic_get(const T* addr)
		{
			T v=*const_cast<T const volatile*>(addr);
			//__memory_barrier();
			return v;
		}

		template<typename T> inline void atomic_set(T* addr, T v)
		{
			//__memory_barrier();
			*const_cast<T volatile*>(addr)=v;
		}

		class basic_task
		{
			public:
				basic_task();
				virtual ~basic_task();
				basic_task* get_next();
				virtual void reset();
				void set_next(basic_task* next);
			private:
				basic_task* next;
		};

		template<class task_type> class task_parallel_queue
		{
			public:
				task_parallel_queue();
				explicit task_parallel_queue(int event_fd);
				~task_parallel_queue();
				task_type* allocate_tmp_node();
				int task_enqueue_signal_notification();
				int task_enqueue();
				void task_dequeue();
				task_type* get_task();
				bool is_empty();
				bool task_wait();
				void set_queue_event_fd(int queue_event_fd);
				void destory_queue();

			private:
				task_type* queue_head; 
				task_type* queue_tail;
				//resevered nodes which are still under I/O
				task_type* queue_tmp_tail;
				//used to allocate temp
				//and make item available later
				task_type* queue_tmp_head;

				size_t length_of_queue;
				
				pthread_mutex_t locker;
				pthread_cond_t queue_empty;

				int queue_event_fd;
		};

		inline basic_task* basic_task::get_next()
		{
			return next;
		}

		inline void basic_task::set_next(basic_task* next_pointer)
		{
			next=next_pointer;
		}

		inline void basic_task::reset()
		{}

		template<class task_type> inline void task_parallel_queue<task_type>::set_queue_event_fd(int queue_event_fd)
		{
			this->queue_event_fd=queue_event_fd;
		}

		template<class task_type> task_parallel_queue<task_type>::task_parallel_queue():
			queue_head(new task_type()),
			queue_tail(queue_head),
			queue_tmp_tail(queue_head),
			queue_tmp_head(queue_head),
			length_of_queue(2),
			locker(),
			queue_empty(),
			queue_event_fd(-1)
		{
			queue_head=new task_type();
			queue_tail->set_next(queue_head);
			queue_head->set_next(queue_tail);
			queue_tmp_head=queue_head;
			pthread_cond_init(&queue_empty, NULL);
			pthread_mutex_init(&locker, NULL);
		}

		template<class task_type> task_parallel_queue<task_type>::task_parallel_queue(int event_fd):
			queue_head(new task_type()),
			queue_tail(queue_head),
			queue_tmp_tail(queue_head),
			queue_tmp_head(queue_head),
			length_of_queue(2),
			locker(),
			queue_empty(),
			queue_event_fd(event_fd)
		{
			queue_head=new task_type();
			queue_tail->set_next(queue_head);
			queue_head->set_next(queue_tail);
			queue_tmp_head=queue_head;
			pthread_cond_init(&queue_empty, NULL);
			pthread_mutex_init(&locker, NULL);
		}

		template<class task_type> task_parallel_queue<task_type>::~task_parallel_queue()
		{}

		template<class task_type> inline bool task_parallel_queue<task_type>::is_empty()
		{
			return queue_tail->get_next() == queue_head;
		}

		template<class task_type> inline void task_parallel_queue<task_type>::task_dequeue()
		{
			_DEBUG("task dequeue\n");
			queue_tmp_tail=static_cast<task_type*>(queue_tmp_tail->get_next());
		}

		template<class task_type> bool task_parallel_queue<task_type>::task_wait()
		{
			static task_type* previous_head=NULL;
			if(previous_head == queue_head || queue_tail->get_next() == queue_head )
			{
				pthread_cond_wait(&queue_empty, &locker);
				previous_head=queue_head;
			}
			return true;
		}

		template<class task_type> task_type* task_parallel_queue<task_type>::get_task()
		{
			while(queue_tail->get_next() == queue_head)
			{
				pthread_cond_wait(&queue_empty, &locker);
			}
			task_type* new_task=static_cast<task_type*>(queue_tail->get_next());
			atomic_set(&queue_tail, static_cast<task_type*>(queue_tail->get_next()));
			return new_task;
		}

		template<class task_type> task_type* task_parallel_queue<task_type>::allocate_tmp_node()
		{
			task_type* ret=NULL;
			if(queue_tmp_tail == queue_tmp_head->get_next())
			{
				_DEBUG("new queue item allocated\nlength of the queue=%ld\n", length_of_queue);
				ret=new task_type();
				ret->set_next(queue_head->get_next());
				queue_head->set_next(ret);
				queue_tmp_head=ret;
				ret=queue_head;
				length_of_queue++;
			}
			else
			{
				_DEBUG("queue item reused\nlength of the queue=%ld\n", length_of_queue);
				ret=queue_head;
				queue_tmp_head=static_cast<task_type*>(queue_head->get_next());
			}
			ret->reset();
			return ret;
		}

		template<class task_type> int task_parallel_queue<task_type>::task_enqueue_signal_notification()
		{
			queue_head=queue_tmp_head;
			pthread_cond_signal(&queue_empty);
			return SUCCESS;
		}

		template<class task_type> int task_parallel_queue<task_type>::task_enqueue()
		{
			queue_head=queue_tmp_head;
			static uint64_t notification=1;
			if(-1 == write(queue_event_fd, &notification, sizeof(uint64_t)))
			{
				perror("write");
				return FAILURE;
			}
			return SUCCESS;
		}

		template<class task_type> void task_parallel_queue<task_type>::destory_queue()
		{
			basic_task* tmp=NULL, *next=queue_head;
			while(queue_tail != next)
			{
				tmp=next;
				next=next->get_next();
				delete tmp;
			}
			return;
		}
	}
}

#endif
