#ifndef CBB_COMMUNICATION_THREAD_H_
#define CBB_COMMUNICATION_THREAD_H_

#include <pthread.h>
#include <set>

#include "CBB_task_parallel.h"

namespace CBB
{
	namespace Common
	{
		int Send_attr(extended_IO_task* output_task, const struct stat* file_stat);
		int Recv_attr(extended_IO_task* new_task, struct stat* file_stat);

		class CBB_communication_thread
		{
			protected:
				CBB_communication_thread()throw(std::runtime_error);
				virtual ~CBB_communication_thread();
				int start_communication_server();
				int stop_communication_server();
				int _add_socket(int socketfd);
				int _add_socket(int socketfd, int op);
				int _delete_socket(int socketfd); 
				virtual int input_from_socket(int socket, task_parallel_queue<extended_IO_task>* output_queue)=0;
				virtual int input_from_producer(task_parallel_queue<extended_IO_task>* input_queue)=0;
				size_t send(extended_IO_task* new_task);
				//size_t send_basic_message(extended_IO_task* new_task);
				//size_t receive_basic_message(int socket, extended_IO_task* new_task);
				size_t receive_message(int socket, extended_IO_task* new_task);
				static void* thread_function(void*);
				//thread wait on queue event;
				static void* queue_wait_function(void*);
				void set_queue(task_parallel_queue<extended_IO_task>* input_queue, task_parallel_queue<extended_IO_task>* output_queue);
				//void set_init_barrier(pthread_barrier_t* init_barrier);
			private:
				//map: socketfd
				typedef std::set<int> socket_pool_t; 

			private:
				int keepAlive;
				int epollfd; 
				socket_pool_t _socket_pool; 

				bool thread_started;
				task_parallel_queue<extended_IO_task>* input_queue;
				task_parallel_queue<extended_IO_task>* output_queue;
				pthread_t communication_thread;
				//pthread_t queue_event_wait_thread;
				int queue_event_fd;
		};

		/*inline size_t CBB_communication_thread::send_basic_message(extended_IO_task* new_task)
		{
			Send(new_task->get_socket(), new_task->get_message_size());
			return Sendv_pre_alloc(new_task->get_socket(), new_task->get_message(), new_task->get_message_size());
		}

		inline size_t CBB_communication_thread::receive_extended_message(extended_IO_task* new_task)
		{
			size_t size;
			if(0 == Recv(new_task->get_socket(), size))
			{
				close(new_task->get_socket());
				return 0;
			}
			new_task->set_extended_data_size(size);
			return size;
			//return Recvv_pre_alloc(new_task->get_socket(), new_task->get_extended_message(), new_task->get_extended_message_size());
		}*/

	}
}

#endif
