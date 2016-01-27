
#ifndef SERVER_H_
#define SERVER_H_

#include <netinet/in.h>
#include <stdexcept>
#include "CBB_task_parallel.h"
#include "CBB_communication_thread.h"
#include "CBB_request_handler.h"
#include "CBB_remote_task.h"
#include "CBB_fault_tolerance.h"

namespace CBB
{
	namespace Common
	{
		class Server:public CBB_communication_thread,
		public CBB_request_handler,
		public CBB_remote_task,
		public CBB_fault_tolerance
		{
			public:
				void stop_server(); 
				int start_server();

			protected:
				Server(int thread_number, int port)throw(std::runtime_error); 
				virtual ~Server(); 
				virtual int input_from_socket(int socket,
						communication_queue_array_t* output)override final;

				virtual int input_from_producer(communication_queue_t* output)override final;
				virtual int output_task_enqueue(extended_IO_task* output_task)override final;
				virtual communication_queue_t* get_communication_queue_from_socket(int socket)override final;
				virtual int node_failure_handler(int socket)override;

				void _init_server()throw(std::runtime_error); 

				virtual int _parse_request(extended_IO_task* new_task)=0;
				virtual std::string _get_real_path(const char* path)const=0;
				virtual std::string _get_real_path(const std::string& path)const=0;
				virtual int remote_task_handler(remote_task* new_task)=0;

				int send_input_for_socket_error(int socket);
				extended_IO_task* init_response_task(extended_IO_task* input_task);
				communication_queue_t* get_communication_input_queue(int index);
				communication_queue_t* get_communication_output_queue(int index);
				int _recv_real_path(extended_IO_task* new_task, std::string& real_path);
				int _recv_real_relative_path(extended_IO_task* new_task, std::string& real_path, std::string &relative_path);
				//id = thread id, temporarily id=0
				extended_IO_task* allocate_output_task(int id);
			private:
				void _setup_socket();

			protected:
				communication_queue_array_t _communication_input_queue;
				communication_queue_array_t _communication_output_queue;
				struct sockaddr_in _server_addr;
				int _port;
				int _server_socket;
				threads_socket_map_t _threads_socket_map;
		}; 

		inline int Server::output_task_enqueue(extended_IO_task* output_task)
		{
			communication_queue_t& output_queue=_communication_output_queue.at(output_task->get_id());
			return output_queue.task_enqueue();
		}

		inline extended_IO_task* Server::allocate_output_task(int id)
		{
			extended_IO_task* output=_communication_output_queue.at(id).allocate_tmp_node();
			output->set_receiver_id(0);
			return output;
		}

		inline communication_queue_t* Server::get_communication_input_queue(int index)
		{
			return &_communication_input_queue.at(index);
		}

		inline communication_queue_t* Server::get_communication_output_queue(int index)
		{
			return &_communication_output_queue.at(index);
		}
	}
}

#endif
