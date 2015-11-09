#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdexcept>

#include "CBB_communication_thread.h"
#include "CBB_connector.h"

namespace CBB
{
	namespace Common
	{
		class Client:public CBB_communication_thread, public CBB_connector
		{
			public:
				Client();
				virtual ~Client();
			protected:
				void stop_client();
				int start_client();
				virtual int input_from_socket(int socket,
						task_parallel_queue<extended_IO_task>* output_queue);
				virtual int input_from_producer(task_parallel_queue<extended_IO_task>* input_queue);
				extended_IO_task* allocate_new_query(int socket);
				int send_query(extended_IO_task* query);
				extended_IO_task* get_query_response(extended_IO_task* query);
				int response_dequeue(extended_IO_task* response);
			private:
				task_parallel_queue<extended_IO_task> _communication_input_queue;
				task_parallel_queue<extended_IO_task> _communication_output_queue;

		}; 

		inline extended_IO_task* Client::allocate_new_query(int socket)
		{
			extended_IO_task* query_task=_communication_output_queue.allocate_tmp_node();
			query_task->set_socket(socket);
			return query_task;
		}

		inline int Client::send_query(extended_IO_task* query)
		{
			_DEBUG("send query to master\n");
			return _communication_output_queue.task_enqueue();
		}

		inline extended_IO_task* Client::get_query_response(extended_IO_task* query)
		{
			extended_IO_task* ret=_communication_input_queue.get_task();
			while(ret->get_socket() != query->get_socket())
			{
				_DEBUG("error!\n");
				ret=_communication_input_queue.get_task();
			}
			return ret;
		}
		
		inline int Client::response_dequeue(extended_IO_task* response)
		{
			_communication_input_queue.task_dequeue();
			return SUCCESS;
		}
	}
}

#endif
