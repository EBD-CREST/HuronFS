#ifndef CBB_TASK_PARALLEL_H_
#define CBB_TASK_PARALLEL_H_

#include <pthread.h>
#include <atomic>
#include <sched.h>
#include <unistd.h>

#include "CBB_mutex_locker.h"
#include "CBB_const.h"
#include "string.h"
#include "CBB_internal.h"

namespace CBB
{
	namespace Common
	{

		inline void yield()
		{
#ifdef SLEEP_YIELD
			sched_yield();
#else
			;
#endif
		}
		class basic_task
		{
			public:
				basic_task()=default;
				basic_task(int id, basic_task* next);
				virtual ~basic_task()=default;
				basic_task* get_next();
				virtual void reset();
				void set_next(basic_task* next);
				virtual void set_id(int id);
				int get_id()const;
			private:
				int id;
				basic_task* next;
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

				void flush_queue()const;

			private:
				std::atomic<task_type*> queue_head; 
				std::atomic<task_type*> queue_tail;
				//reserved nodes which are still under I/O
				std::atomic<task_type*> queue_tmp_tail;
				//used to allocate temp
				//and make item available later
				std::atomic<task_type*> queue_tmp_head;

				size_t length_of_queue;
				
				pthread_mutex_t lock;
				pthread_cond_t queue_empty;

				int queue_event_fd;
				int queue_id;
				pthread_mutex_t queue_lock;
		};

		inline basic_task::basic_task(int id, basic_task* next):
			id(id),
			next(next)
		{}

		inline basic_task* basic_task::get_next()
		{
			return this->next;
		}

		inline void basic_task::set_next(basic_task* next_pointer)
		{
			this->next=next_pointer;
		}

		inline void basic_task::reset()
		{}

		inline void basic_task::set_id(int id)
		{
			this->id=id;
		}

		inline int basic_task::get_id()const
		{
			return this->id;
		}

		template<class task_type> inline void task_parallel_queue<task_type>::set_queue_event_fd(int queue_event_fd)
		{
			this->queue_event_fd=queue_event_fd;
		}

		template<class task_type> task_parallel_queue<task_type>::task_parallel_queue():
			queue_head(nullptr),
			queue_tail(new task_type()),
			queue_tmp_tail(queue_tail.load(std::memory_order_relaxed)),
			queue_tmp_head(nullptr),
			length_of_queue(2),
			lock(),
			queue_empty(),
			queue_event_fd(-1),
			queue_id(-1),
			queue_lock()
		{
			queue_head.store(new task_type(), std::memory_order_relaxed);
			queue_tail.load()->set_next(queue_head.load(std::memory_order_relaxed));
			queue_head.load()->set_next(queue_tail.load(std::memory_order_relaxed));
			queue_tmp_head.store(queue_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
			pthread_cond_init(&queue_empty, nullptr);
			pthread_mutex_init(&lock, nullptr);
		}

		template<class task_type> task_parallel_queue<task_type>::task_parallel_queue(int id):
			queue_head(nullptr),
			queue_tail(new task_type()),
			queue_tmp_tail(queue_tail.load(std::memory_order_relaxed)),
			queue_tmp_head(nullptr),
			length_of_queue(2),
			lock(),
			queue_empty(),
			queue_event_fd(-1),
			queue_id(id),
			queue_lock()
		{
			queue_head.store(new task_type(), std::memory_order_relaxed);
			queue_tail.load()->set_next(queue_head.load(std::memory_order_relaxed));
			queue_head.load()->set_next(queue_tail.load(std::memory_order_relaxed));
			queue_tmp_head.store(queue_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
			pthread_cond_init(&queue_empty, nullptr);
			pthread_mutex_init(&lock, nullptr);
		}

		template<class task_type> task_parallel_queue<task_type>::task_parallel_queue(int id, int event_fd):
			queue_head(nullptr),
			queue_tail(new task_type()),
			queue_tmp_tail(queue_tail.load(std::memory_order_relaxed)),
			queue_tmp_head(nullptr),
			length_of_queue(2),
			lock(),
			queue_empty(),
			queue_event_fd(event_fd),
			queue_id(id),
			queue_lock()
		{
			queue_head.store(new task_type(), std::memory_order_relaxed);
			queue_tail.load()->set_next(queue_head.load(std::memory_order_relaxed));
			queue_head.load()->set_next(queue_tail.load(std::memory_order_relaxed));
			queue_tmp_head.store(queue_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
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
			return this->queue_tail.load()->get_next() == this->queue_head.load();
		}

		template<class task_type> inline void task_parallel_queue<task_type>::task_dequeue()
		{
			task_type* current_tmp_tail=this->queue_tmp_tail.load();
			this->queue_tmp_tail.store(static_cast<task_type*>(current_tmp_tail->get_next()));
		}

		template<class task_type> bool task_parallel_queue<task_type>::task_wait()
		{
			static task_type* previous_head=nullptr;

			task_type* current_head=this->queue_head.load();
			task_type* current_tail=this->queue_tail.load();
			
			if(previous_head == current_head || is_empty())
			{
#ifdef BUSY_WAIT
				while(is_empty())
				{
					yield();
				}
#else
				pthread_cond_wait(&queue_empty, &lock);
#endif
				previous_head=current_head;
			}
			return true;
		}

		template<class task_type> task_type* task_parallel_queue<task_type>::get_task()
		{
			while(is_empty())
			{
#ifdef BUSY_WAIT
				yield();
#else
				pthread_cond_wait(&queue_empty, &lock);
#endif
			}
			task_type* new_task=static_cast<task_type*>(queue_tail.load()->get_next());
			this->queue_tail.store(new_task);
			return new_task;
		}

		template<class task_type> task_type* task_parallel_queue<task_type>::allocate_tmp_node()
		{
			task_type* ret=nullptr;
			if(queue_tmp_tail.load() == queue_tmp_head.load()->get_next())
			{
				_DEBUG("new queue item allocated length of the queue=%ld\n", length_of_queue);
				ret=new task_type(queue_id,
						static_cast<task_type*>(
							queue_head.load(
								std::memory_order_relaxed)->get_next()));
				queue_head.load(std::memory_order_relaxed)->set_next(ret);
				queue_tmp_head.store(ret);
				ret=queue_head.load(std::memory_order_relaxed);
				length_of_queue++;
			}
			else
			{
				_DEBUG("queue item reused length of the queue=%ld\n", length_of_queue);
				ret=queue_head.load();
				queue_tmp_head.store(static_cast<task_type*>(ret->get_next()));
			}
			ret->reset();
			return ret;
		}

		template<class task_type> int task_parallel_queue<task_type>::task_enqueue_signal_notification()
		{
			this->queue_head.store(this->queue_tmp_head.load());
#ifndef BUSY_WAIT
			pthread_cond_signal(&queue_empty);
#endif
			return SUCCESS;
		}

		template<class task_type> int task_parallel_queue<task_type>::task_enqueue()
		{
			this->queue_head.store(this->queue_tmp_head.load());
			static uint64_t notification=1;

			_DEBUG("task enqueue\n");
			if(-1 == write(this->queue_event_fd, &notification, sizeof(uint64_t)))
			{
				perror("write");
				return FAILURE;
			}
			return SUCCESS;
		}

		template<class task_type> void task_parallel_queue<task_type>::destory_queue()
		{
			basic_task* tmp=nullptr, *next=queue_head.load();
			while(this->queue_tail.load() != next)
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
			basic_task* next=queue_head.load();

			while(this->queue_tail.load() != next)
			{
				next->set_id(id);
				next=next->get_next();
			}
			this->queue_tail.load()->set_id(id);
			return;
		}

		template<class task_type> void task_parallel_queue<task_type>::putback_tmp_node()
		{
			queue_tmp_head.store(queue_head.load());
		}

		template<class task_type> void task_parallel_queue<task_type>::flush_queue()const
		{
			basic_task* next=queue_head.load()->get_next();
			_DEBUG("queue entry %p\n", queue_head.load());
			while(queue_head.load() != next)
			{
				_DEBUG("queue entry %p\n", next);
				next=next->get_next();
			}
			_DEBUG("end of queue entry\n");
			//_DEBUG("queue head %p, queue tmp head %p, queue tail %p, queue tmp tail %p\n", queue_head, queue_tmp_head, queue_tail, queue_tmp_tail);
			return;
		}

	}
}

#endif
