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

		class IO_task 
		{
			public:
				IO_task();
				//~task();
				template<typename T> size_t push_back(const T& value);
				template<typename T> size_t push_backv(const T& value, size_t num);
				size_t push_back_string(const unsigned char* string);
				size_t push_back_string(const char* string);
				size_t push_back_string(const char* string, size_t size);
				template<typename T> size_t pop(T& var);
				template<typename T> size_t popv(T* var, size_t num);
				template<typename T> size_t pop_uncopy(T** var);
				template<typename T> size_t popv_uncopy(T** var, size_t num);
				template<typename T> size_t do_pop(T** value, size_t num);
				size_t do_push(const void* value, size_t num);
				size_t pop_string(unsigned char** var);
				size_t pop_string(char** var);
				//void reset_for_read();
				//void reset_for_write();
				void reset();
				int get_socket();
				void set_socket(int socket);
				int get_mode();
				void set_mode(int mode);
				unsigned char* get_message();
				void set_message_size(size_t message_size);
				size_t get_message_size();
				IO_task* get_next();
				void set_next(IO_task* next);
				unsigned char* get_extended_message();
				size_t get_extended_message_size();
				void free_extended_message();
				void set_extended_message_size(size_t size);
				int set_extended_message(void* external_message, size_t size);

			private:
				//send or receive
				int mode;
				int socket;
				size_t message_size;
				unsigned char basic_message[MAX_BASIC_MESSAGE_SIZE];
				size_t extended_message_size;
				unsigned char* extended_message;
				IO_task* next;
				size_t current_point;
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

			private:
				task_type* queue_head; 
				task_type* queue_tail;
				//resevered nodes which are still under I/O
				task_type* queue_tmp_tail;
				//used to allocate temp
				//and make item available later
				task_type* queue_tmp_head;
				
				pthread_mutex_t locker;
				pthread_cond_t queue_empty;

				int queue_event_fd;
		};

		IO_task* init_response_task(IO_task* input_task, task_parallel_queue<IO_task>* output_queue);

		template<typename T> size_t IO_task::push_back(const T& value)
		{
			return push_backv(value, 1);
		}

		template<typename T> inline size_t IO_task::push_backv(const T& value, size_t num)
		{
			return do_push(&value, num*sizeof(T));
		}

		inline size_t IO_task::do_push(const void* value, size_t num)
		{
			memcpy(basic_message + message_size, value, num);
			message_size += num;
			return num;
		}

		template<typename T> inline size_t IO_task::pop_uncopy(T** var)
		{
			return popv_uncopy(var, 1);
		}

		template<typename T> inline size_t IO_task::popv_uncopy(T** var, size_t num)
		{
			return do_pop(var, num*sizeof(T));
		}

		template<typename T> inline size_t IO_task::do_pop(T** var, size_t num)
		{
			*var=reinterpret_cast<T*>(basic_message + current_point);
			current_point += num;
			return num;
		}

		template<typename T> inline size_t IO_task::pop(T& var)
		{
			return popv(&var, 1); } 
		template<typename T> inline size_t IO_task::popv(T* var, size_t num)
		{
			T* tmp=NULL;
			size_t ret=popv_uncopy(&tmp, num);
			memcpy(var, tmp, ret);
			return ret;
		}

		inline size_t IO_task::push_back_string(const unsigned char* string)
		{
			return push_back_string(reinterpret_cast<const char*>(string));
		}

		inline size_t IO_task::push_back_string(const char* string)
		{
			size_t len=strlen(string)+1;
			push_back(len);
			do_push(string, len*sizeof(unsigned char));
			return len;
		}

		inline size_t IO_task::push_back_string(const char* string, size_t size)
		{
			push_back(size+1);
			do_push(string, (size+1)*sizeof(unsigned char));
			return size+1;
		}

		inline size_t IO_task::pop_string(unsigned char** var)
		{
			size_t len;
			pop(len);
			do_pop(var, len*sizeof(unsigned char));
			return len;
		}

		inline size_t IO_task::pop_string(char** var)
		{
			return pop_string(reinterpret_cast<unsigned char**>(var));
		}

		inline unsigned char* IO_task::get_message()
		{
			return basic_message;
		}

		inline int IO_task::get_socket()
		{
			return socket;
		}

		//inline void IO_task::reset_for_read()
		//{
		//	current_point=0;
		//}

		inline void IO_task::reset()
		{
			message_size=0;
			extended_message_size=0;
			current_point=0;
			mode=SEND;
		}

		inline void IO_task::set_socket(int socket)
		{
			this->socket=socket;
		}

		inline int IO_task::get_mode()
		{
			return this->mode;
		}

		inline void IO_task::set_mode(int mode)
		{
			this->mode=mode;
		}

		inline size_t IO_task::get_message_size()
		{
			return this->message_size;
		}

		inline void IO_task::set_message_size(size_t message_size)
		{
			this->message_size=message_size;
		}

		inline unsigned char* IO_task::get_extended_message()
		{
			return extended_message;
		}

		inline IO_task* IO_task::get_next()
		{
			return next;
		}

		inline void IO_task::set_next(IO_task* next_pointer)
		{
			next=next_pointer;
		}

		inline size_t IO_task::get_extended_message_size()
		{
			return extended_message_size;
		}

		inline void IO_task::set_extended_message_size(size_t size)
		{
			extended_message_size=size;
		}

		inline void IO_task::free_extended_message()
		{
			extended_message = NULL;
			extended_message_size = 0;
		}

		template<class task_type> inline bool task_parallel_queue<task_type>::is_empty()
		{
			return queue_tail->get_next() == queue_head;
		}

		template<class task_type> inline void task_parallel_queue<task_type>::task_dequeue()
		{
			_DEBUG("task dequeue\n");
			queue_tmp_tail=queue_tmp_tail->get_next();
		}
		
		template<class task_type> task_parallel_queue<task_type>::task_parallel_queue():
			queue_head(new task_type()),
			queue_tail(queue_head),
			queue_tmp_tail(queue_head),
			queue_tmp_head(queue_head),
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
			task_type* new_task=queue_tail->get_next();
			atomic_set(&queue_tail, queue_tail->get_next());
			return new_task;
		}

		template<class task_type> task_type* task_parallel_queue<task_type>::allocate_tmp_node()
		{
			task_type* ret=NULL;
			if(queue_tmp_tail == queue_tmp_head->get_next())
			{
				ret=new task_type();
				ret->set_next(queue_head->get_next());
				queue_head->set_next(ret);
				queue_tmp_head=ret;
				ret=queue_head;
			}
			else
			{
				ret=queue_head;
				queue_tmp_head=queue_head->get_next();
				//atomic_set(&queue_head, queue_head->get_next());
			}
			ret->reset();

			return ret;
		}

		template<class task_type> int task_parallel_queue<task_type>::task_enqueue_signal_notification()
		{
			task_type* original_head=queue_head;
			queue_head=queue_tmp_head;
			if(queue_tail->get_next() == original_head)
			{
				pthread_cond_signal(&queue_empty);
			}
			return SUCCESS;
		}

		template<class task_type> int task_parallel_queue<task_type>::task_enqueue()
		{
			task_type* original_head=queue_head;
			queue_head=queue_tmp_head;
			static uint64_t notification=1;
			if(queue_tail->get_next() == original_head)
			{
				_DEBUG("task enqueue\n");
				if(-1 == write(queue_event_fd, &notification, sizeof(uint64_t)))
				{
					perror("write");
					return FAILURE;
				}
			}
			return SUCCESS;
		}

		template<class task_type> void task_parallel_queue<task_type>::set_queue_event_fd(int queue_event_fd)
		{
			this->queue_event_fd=queue_event_fd;
		}

		inline IO_task* init_response_task(IO_task* input_task, task_parallel_queue<IO_task>* output_queue)
		{
			IO_task* output=output_queue->allocate_tmp_node();
			output->set_socket(input_task->get_socket());
			return output;
		}

		inline int IO_task::set_extended_message(void* extended_message_p, size_t size)
		{
			extended_message = static_cast<unsigned char*>(extended_message_p);
			extended_message_size = size;
			return SUCCESS;
		}
	}
}

#endif
