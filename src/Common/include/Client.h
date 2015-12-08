#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdexcept>
#include <vector>

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
						communication_queue_array_t* output_queue)override;
				virtual int input_from_producer(communication_queue_t* input_queue)override;
				virtual int output_task_enqueue(extended_IO_task* output_task)override;

				communication_queue_t* get_new_communication_queue();
				int release_communication_queue(communication_queue_t* queue);
				int release_communication_queue(extended_IO_task* task);
				extended_IO_task* allocate_new_query(int socket);
				extended_IO_task* allocate_new_query_preallocated(int socket, int id);
				int send_query(extended_IO_task* query);
				extended_IO_task* get_query_response(extended_IO_task* query);
				int response_dequeue(extended_IO_task* response);
				communication_queue_t* get_input_queue_from_query(extended_IO_task* query);
				communication_queue_t* get_output_queue_from_query(extended_IO_task* query);
			private:
				communication_queue_array_t _communication_input_queue;
				communication_queue_array_t _communication_output_queue;

		}; 

		inline extended_IO_task* Client::allocate_new_query(int socket)
		{
			communication_queue_t* new_queue=get_new_communication_queue();
			_DEBUG("lock queue %p\n", new_queue);
			extended_IO_task* query_task=new_queue->allocate_tmp_node();
			query_task->set_socket(socket);
			query_task->set_id_to_be_sent(query_task->get_id());
			return query_task;
		}

		inline extended_IO_task* Client::allocate_new_query_preallocated(int socket, int id)
		{
			communication_queue_t& new_queue=_communication_output_queue.at(id);
			extended_IO_task* query_task=new_queue.allocate_tmp_node();
			query_task->set_socket(socket);
			return query_task;
		}

		inline int Client::send_query(extended_IO_task* query)
		{
			_DEBUG("send query to master\n");
			return get_output_queue_from_query(query)->task_enqueue();
		}

		inline extended_IO_task* Client::get_query_response(extended_IO_task* query)
		{
			extended_IO_task* ret=get_input_queue_from_query(query)->get_task();
			/*while(ret->get_socket() != query->get_socket())
			{
				_DEBUG("error!\n");
				ret=_communication_input_queue.get_task();
			}*/
			return ret;
		}
		
		inline int Client::response_dequeue(extended_IO_task* response)
		{
			get_input_queue_from_query(response)->task_dequeue();
			return SUCCESS;
		}

		inline communication_queue_t* Client::get_input_queue_from_query(extended_IO_task* query)
		{
			int id=query->get_id();
			return &_communication_input_queue[id];
		}

		inline communication_queue_t* Client::get_output_queue_from_query(extended_IO_task* query)
		{
			int id=query->get_id();
			return &_communication_output_queue[id];
		}

		inline int Client::output_task_enqueue(extended_IO_task* output_task)
		{
			communication_queue_t& output_queue=_communication_output_queue.at(output_task->get_id());
			return output_queue.task_enqueue();
		}

		inline int Client::release_communication_queue(communication_queue_t* queue)
		{
			_DEBUG("release queue %p\n", queue);
			return queue->unlock_queue();
		}

		inline int Client::release_communication_queue(extended_IO_task* task)
		{
			communication_queue_t& output_queue=_communication_output_queue.at(task->get_id());
			return release_communication_queue(&output_queue);
		}
	}
}

#endif
