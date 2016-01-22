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

		class basic_task
		{
			public:
				basic_task()=default;
				virtual ~basic_task()=default;
				basic_task* get_next();
				virtual void reset();
				void set_next(basic_task* next);
				void set_id(int id);
				int get_id()const;
			private:
				basic_task* next;
				int id;
		};

		template<class task_type> class task_parallel_queue
		{
			public:
				task_parallel_queue();
				explicit task_parallel_queue(int id);
				task_parallel_queue(int id, int event_fd);
				~task_parallel_queue();
				task_type* allocate_tmp_node();
				void putback_tmp_node();
				int task_enqueue_signal_notification();
				int task_enqueue();
				void task_dequeue();
				task_type* get_task();
				bool is_empty();
				bool task_wait();
				void set_queue_event_fd(int queue_event_fd);
				void destory_queue();
				int try_lock_queue();
				int unlock_queue();
				void set_queue_id(int id);

			private:
				task_type* queue_head; 
				task_type* queue_tail;
				//resevered nodes which are still under I/O
				task_type* queue_tmp_tail;
				//used to allocate temp
				//and make item available later
				task_type* queue_tmp_head;

				size_t length_of_queue;
				
				pthread_mutex_t lock;
				pthread_cond_t queue_empty;

				int queue_event_fd;
				int queue_id;
				pthread_mutex_t queue_lock;
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

		inline void basic_task::set_id(int id)
		{
			this->id=id;
		}

		inline int basic_task::get_id()const
		{
			return id;
		}

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
			lock(),
			queue_empty(),
			queue_event_fd(-1),
			queue_id(-1),
			queue_lock()
		{
			queue_head=new task_type();
			queue_tail->set_next(queue_head);
			queue_head->set_next(queue_tail);
			queue_tmp_head=queue_head;
			pthread_cond_init(&queue_empty, nullptr);
			pthread_mutex_init(&lock, nullptr);
		}

		template<class task_type> task_parallel_queue<task_type>::task_parallel_queue(int id):
			queue_head(new task_type()),
			queue_tail(queue_head),
			queue_tmp_tail(queue_head),
			queue_tmp_head(queue_head),
			length_of_queue(2),
			lock(),
			queue_empty(),
			queue_event_fd(-1),
			queue_id(id),
			queue_lock()
		{
			queue_head=new task_type();
			queue_tail->set_next(queue_head);
			queue_head->set_next(queue_tail);
			queue_tmp_head=queue_head;
			pthread_cond_init(&queue_empty, nullptr);
			pthread_mutex_init(&lock, nullptr);
		}

		template<class task_type> task_parallel_queue<task_type>::task_parallel_queue(int id, int event_fd):
			queue_head(new task_type()),
			queue_tail(queue_head),
			queue_tmp_tail(queue_head),
			queue_tmp_head(queue_head),
			length_of_queue(2),
			lock(),
			queue_empty(),
			queue_event_fd(event_fd),
			queue_id(id),
			queue_lock()
		{
			queue_head=new task_type();
			queue_tail->set_next(queue_head);
			queue_head->set_next(queue_tail);
			queue_tmp_head=queue_head;
			pthread_cond_init(&queue_empty, nullptr);
			pthread_mutex_init(&lock, nullptr);
		}

		template<class task_type> task_parallel_queue<task_type>::~task_parallel_queue()
		{
			destory_queue();
		}

		template<class task_type> inline int task_parallel_queue<task_type>::try_lock_queue()
		{
			return pthread_mutex_trylock(&queue_lock);
		}

		template<class task_type> inline int task_parallel_queue<task_type>::unlock_queue()
		{
			return pthread_mutex_unlock(&queue_lock);
		}

		template<class task_type> inline bool task_parallel_queue<task_type>::is_empty()
		{
			return queue_tail->get_next() == queue_head;
		}

		template<class task_type> inline void task_parallel_queue<task_type>::task_dequeue()
		{
			queue_tmp_tail=static_cast<task_type*>(queue_tmp_tail->get_next());
		}

		template<class task_type> bool task_parallel_queue<task_type>::task_wait()
		{
			static task_type* previous_head=nullptr;
			if(previous_head == queue_head || queue_tail->get_next() == queue_head )
			{
				pthread_cond_wait(&queue_empty, &lock);
				previous_head=queue_head;
			}
			return true;
		}

		template<class task_type> task_type* task_parallel_queue<task_type>::get_task()
		{
			while(queue_tail->get_next() == queue_head)
			{
				pthread_cond_wait(&queue_empty, &lock);
			}
			task_type* new_task=static_cast<task_type*>(queue_tail->get_next());
			queue_tail=static_cast<task_type*>(queue_tail->get_next());
			return new_task;
		}

		template<class task_type> task_type* task_parallel_queue<task_type>::allocate_tmp_node()
		{
			task_type* ret=nullptr;
			if(queue_tmp_tail == queue_tmp_head->get_next())
			{
				_DEBUG("new queue item allocated length of the queue=%ld\n", length_of_queue);
				ret=new task_type();
				ret->set_next(queue_head->get_next());
				ret->set_id(queue_id);
				queue_head->set_next(ret);
				queue_tmp_head=ret;
				ret=queue_head;
				length_of_queue++;
			}
			else
			{
				_DEBUG("queue item reused length of the queue=%ld\n", length_of_queue);
				ret=queue_head;
				queue_tmp_head=static_cast<task_type*>(queue_head->get_next());
			}
			ret->reset();
			return ret;
		}

		template<class task_type> int task_parallel_queue<task_type>::task_enqueue_signal_notification()
		{
			queue_head=queue_tmp_head;
			_DEBUG("send to queue %p\n",this);
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
			basic_task* tmp=nullptr, *next=queue_head;
			while(queue_tail != next)
			{
				tmp=next;
				next=next->get_next();
				delete tmp;
			}
			delete queue_tail;
			return;
		}

		template<class task_type>void task_parallel_queue<task_type>::set_queue_id(int id)
		{
			queue_id=id;
			basic_task* next=queue_head;
			while(queue_tail != next)
			{
				next->set_id(id);
				next=next->get_next();
			}
			queue_tail->set_id(id);
			return;
		}

		template<class task_type> void task_parallel_queue<task_type>::putback_tmp_node()
		{
			queue_tmp_head=queue_head;
		}

	}
}

#endif
