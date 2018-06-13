/*
 * Copyright (c) 2017, Tokyo Institute of Technology
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
 * All rights reserved. 
 * 
 * This file is part of HuronFS.
 * 
 * Please also read the file "LICENSE" included in this package for 
 * Our Notice and GNU Lesser General Public License.
 * 
 * This program is free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License (as published by the Free Software 
 * Foundation) version 2.1 dated February 1999. 
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef CBB_TASK_PARALLEL_H_
#define CBB_TASK_PARALLEL_H_

#include <pthread.h>
#include <atomic>
#include <sched.h>
#include <unistd.h>

#include "CBB_mutex_lock.h"
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
				int 		id;
				basic_task* 	next;
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
				int task_enqueue_no_notification();
				task_type* task_dequeue();
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
				// the order in the queue is queue_tail -> ... ->  queue_head -> ... -> queue_tmp_head
				task_type* queue_head; 
				task_type* queue_tail;
				//reserved nodes which are still under I/O
				//std::atomic<task_type*> queue_tmp_tail;
				//used to allocate temp
				//and make item available later
				task_type* queue_tmp_head;

				size_t 			length_of_queue;
				
				pthread_mutex_t 	lock;
				pthread_cond_t 		queue_empty;

				int 			queue_event_fd;
				int 			queue_id;
				pthread_mutex_t 	queue_lock;
		};

		inline basic_task::
			basic_task(int id, basic_task* next):
			id(id),
			next(next)
		{}

		inline basic_task* basic_task::
			get_next()
		{
			return this->next;
		}

		inline void basic_task::
			set_next(basic_task* next_pointer)
		{
			this->next=next_pointer;
		}

		inline void basic_task::
			reset()
		{}

		inline void basic_task::
			set_id(int id)
		{
			this->id=id;
		}

		inline int basic_task::
			get_id()const
		{
			return this->id;
		}

		template<class task_type> inline void task_parallel_queue<task_type>::
			set_queue_event_fd(int queue_event_fd)
		{
			this->queue_event_fd=queue_event_fd;
		}

		template<class task_type> task_parallel_queue<task_type>::
			task_parallel_queue():
			queue_head(nullptr),
			queue_tail(new task_type()),
			//queue_tmp_tail(queue_tail.load(std::memory_order_relaxed)),
			queue_tmp_head(nullptr),
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

		template<class task_type> task_parallel_queue<task_type>::
			task_parallel_queue(int id):
			queue_head(nullptr),
			queue_tail(new task_type()),
			//queue_tmp_tail(queue_tail.load(std::memory_order_relaxed)),
			queue_tmp_head(nullptr),
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

		template<class task_type> task_parallel_queue<task_type>::
			task_parallel_queue(int id, int event_fd):
			queue_head(nullptr),
			queue_tail(new task_type()),
			//queue_tmp_tail(queue_tail.load(std::memory_order_relaxed)),
			queue_tmp_head(nullptr),
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

		template<class task_type> task_parallel_queue<task_type>::
			~task_parallel_queue()
		{
			destory_queue();
		}

		template<class task_type> inline int task_parallel_queue<task_type>::
			try_lock_queue()
		{
			return pthread_mutex_trylock(&queue_lock);
		}

		template<class task_type> inline int task_parallel_queue<task_type>::
			unlock_queue()
		{
			return pthread_mutex_unlock(&queue_lock);
		}

		template<class task_type> inline bool task_parallel_queue<task_type>::
			is_empty()
		{
			return this->queue_tail->get_next() == this->queue_head;
		}

		template<class task_type> inline task_type* task_parallel_queue<task_type>::
			task_dequeue()
		{
			//this->queue_tail.store(static_cast<task_type*>(this->queue_tail.load()->get_next()));
			_DEBUG("task dequeue on queue %p task %p\n", this, this->queue_tail);
			//_DEBUG("current_tail %p current_head %p\n", this->queue_tail.load(), this->queue_head.load());
			/*task_type* current_tmp_tail=this->queue_tmp_tail.load();
			if(this->queue_tmp_tail.load() == this->queue_tail.load())
			{
				this->queue_tmp_tail.store(static_cast<task_type*>(current_tmp_tail->get_next()));
				this->queue_tail.store(this->queue_tmp_tail.load());
			}
			else
			{
				this->queue_tmp_tail.store(static_cast<task_type*>(current_tmp_tail->get_next()));
			}*/

			return this->queue_tail;
		}

		template<class task_type> bool task_parallel_queue<task_type>::
			task_wait()
		{
			static task_type* previous_head=nullptr;

			task_type* current_head=this->queue_head;
			task_type* current_tail=this->queue_tail;
			
			if(previous_head == current_head || is_empty())
			{
				while(is_empty())
				{
					yield();
				}
				previous_head=current_head;
			}
			return true;
		}

		template<class task_type> task_type* task_parallel_queue<task_type>::
			get_task()
		{
			while(is_empty())
			{
				yield();
			}
			task_type* new_task=static_cast<task_type*>(queue_tail->get_next());
			this->queue_tail=new_task;
			_DEBUG("get task %p from queue %p\n", new_task, this);
			return new_task;
		}

		template<class task_type> task_type* task_parallel_queue<task_type>::
			allocate_tmp_node()
		{
			task_type* ret=queue_tmp_head;
			//if(queue_tmp_tail.load() == queue_tmp_head.load()->get_next())
			if(queue_tail == queue_tmp_head->get_next())
			{
				_DEBUG("new queue item allocated length of the queue=%ld\n", length_of_queue);
				task_type* new_node=new task_type(queue_id,
						static_cast<task_type*>(
							queue_tmp_head->get_next()));
				queue_tmp_head->set_next(new_node);
				queue_tmp_head=new_node;
				length_of_queue++;
			}
			else
			{
				_DEBUG("queue item reused length of the queue=%ld\n", length_of_queue);
				queue_tmp_head=static_cast<task_type*>(ret->get_next());
			}

			ret->reset();
			_DEBUG("allocated node is %p\n", ret);
			return ret;
		}

		template<class task_type> int task_parallel_queue<task_type>::
			task_enqueue_signal_notification()
		{
			_DEBUG("task enqueue of queue %p task %p\n", this, queue_head);
			_DEBUG("queue %p head %p tail %p tmp head %p\n", this, queue_head, queue_tail, queue_tmp_head);
			
			this->queue_head=static_cast<task_type*>(
						this->queue_head->get_next());
			return SUCCESS;
		}

		template<class task_type> int task_parallel_queue<task_type>::
			task_enqueue_no_notification()
		{
			_DEBUG("task enqueue of queue %p task %p\n", this, queue_head);
			_DEBUG("queue %p head %p tail %p tmp head %p\n", this, queue_head, queue_tail, queue_tmp_head);
			this->queue_head=static_cast<task_type*>(this->queue_head->get_next());
			return SUCCESS;
		}

		template<class task_type> int task_parallel_queue<task_type>::
			task_enqueue()
		{
			_DEBUG("task enqueue of queue %p task %p\n", this, queue_head);
			_DEBUG("queue %p head %p tail %p tmp head %p\n", this, queue_head, queue_tail, queue_tmp_head);
			this->queue_head=static_cast<task_type*>(this->queue_head->get_next());
			static uint64_t notification=1;

			if(-1 == write(this->queue_event_fd, &notification, sizeof(uint64_t)))
			{
				perror("enqueue notification");
				_DEBUG("queue fd %d\n", this->queue_event_fd);
				return FAILURE;
			}
			
			return SUCCESS;
		}

		template<class task_type> void task_parallel_queue<task_type>::
			destory_queue()
		{
			basic_task* tmp=nullptr, *next=queue_head;

			if(nullptr == next || nullptr == queue_tail)
			{
				//queue is empty
				return;
			}
			while(this->queue_tail != next)
			{
				tmp=next;
				next=next->get_next();
				delete tmp;
			}
			delete queue_tail;
			queue_head=nullptr;
			queue_tail=nullptr;
			return;
		}

		template<class task_type>void task_parallel_queue<task_type>::
			set_queue_id(int id)
		{
			queue_id=id;
			basic_task* next=queue_head;

			while(this->queue_tail != next)
			{
				next->set_id(id);
				next=next->get_next();
			}
			this->queue_tail->set_id(id);
			return;
		}

		template<class task_type> void task_parallel_queue<task_type>::
			putback_tmp_node()
		{
			queue_tmp_head=queue_head;
		}

		template<class task_type> void task_parallel_queue<task_type>::
			flush_queue()const
		{
			basic_task* next=queue_head->get_next();
			_DEBUG("queue entry %p\n", queue_head);
			while(queue_head != next)
			{
				_DEBUG("queue entry %p\n", next);
				next=next->get_next();
			}
			_DEBUG("end of queue entry\n");
			return;
		}

	}
}

#endif
