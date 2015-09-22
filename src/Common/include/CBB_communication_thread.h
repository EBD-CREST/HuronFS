#ifndef CBB_COMMUNICATION_THREAD_H_
#define CBB_COMMUNICATION_THREAD_H_

#include <pthread.h>
#include <set>

#include "CBB_task_parallel.h"

namespace CBB
{
	namespace Common
	{
		int Send_attr(IO_task* output_task, const struct stat* file_stat);
		int Recv_attr(IO_task* new_task, struct stat* file_stat);

		/*struct thread_args
		{
			public:
				thread_args(int (*function)(void*),
						void *args);
						//pthread_barrier_t* barrier);
				thread_args();
				int (*function)(void*);
				void* args;
				//pthread_barrier_t* barrier;
		};*/

		class CBB_communication_thread
		{
			protected:
				CBB_communication_thread()throw(std::runtime_error);
				virtual ~CBB_communication_thread();
				//int start_communication_server(CBB_communication_thread* obj, int (*thread_function)(void*) );
				int start_communication_server();
				int stop_communication_server();
				int _add_socket(int socketfd);
				int _add_socket(int socketfd, int op);
				int _delete_socket(int socketfd); 
				virtual int input_from_socket(int socket, task_parallel_queue<IO_task>* output_queue)=0;
				virtual int input_from_producer(task_parallel_queue<IO_task>* input_queue)=0;
				size_t send(IO_task* new_task);
				size_t send_basic_message(IO_task* new_task);
				size_t receive_basic_message(int socket, IO_task* new_task);
				size_t receive_extended_message(IO_task* new_task);
				static void* thread_function(void*);
				//thread wait on queue event;
				static void* queue_wait_function(void*);
				void set_queue(task_parallel_queue<IO_task>* input_queue, task_parallel_queue<IO_task>* output_queue);
				//void set_init_barrier(pthread_barrier_t* init_barrier);
			private:
				//map: socketfd
				typedef std::set<int> socket_pool_t; 

			private:
				int keepAlive;
				int epollfd; 
				socket_pool_t _socket_pool; 

				bool thread_started;
				task_parallel_queue<IO_task>* input_queue;
				task_parallel_queue<IO_task>* output_queue;
				pthread_t communication_thread;
				//pthread_t queue_event_wait_thread;
				int queue_event_fd;
		};

		inline size_t CBB_communication_thread::send_basic_message(IO_task* new_task)
		{
			Send(new_task->get_socket(), new_task->get_message_size());
			return Sendv_pre_alloc(new_task->get_socket(), new_task->get_message(), new_task->get_message_size());
		}

		inline size_t CBB_communication_thread::receive_basic_message(int socket, IO_task* new_task)
		{
			size_t size=0;
			int ret=0;
			Recv(socket, size);
			_DEBUG("receive basic message size=%ld\n", size);
			new_task->set_socket(socket);
			new_task->set_message_size(size);
			ret=Recvv_pre_alloc(socket, new_task->get_message(),size); 
			//fwrite(new_task->get_message(), new_task->get_message_size(), sizeof(char), stderr);
			return ret;
		}

		inline size_t CBB_communication_thread::receive_extended_message(IO_task* new_task)
		{
			size_t size;
			Recv(new_task->get_socket(), size);
			new_task->set_extended_message_size(size);
			return Recvv_pre_alloc(new_task->get_socket(), new_task->get_extended_message(), new_task->get_extended_message_size());
		}

		/*inline void CBB_communication_thread::set_init_barrier(pthread_barrier_t* init_barrier)
		{
			this->init_barrier=init_barrier;
		}*/
	}
}

#endif
