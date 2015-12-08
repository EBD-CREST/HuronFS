#ifndef CBB_COMMUNICATION_THREAD_H_
#define CBB_COMMUNICATION_THREAD_H_

#include <pthread.h>
#include <set>
#include <vector>
#include <map>

#include "CBB_task_parallel.h"

namespace CBB
{
	namespace Common
	{

		class basic_IO_task:public basic_task
		{
			public:
				basic_IO_task();
				virtual ~basic_IO_task()=default;
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
				virtual void reset();
				int get_socket()const;
				void set_socket(int socket);
				unsigned char* get_message();
				void set_message_size(size_t message_size);
				size_t get_message_size()const;
				int get_receiver_id()const;
				void set_receiver_id(int id);
				int get_id_to_be_sent()const;
				void set_id_to_be_sent(int id);
			private:
				int socket;
				//remote id refers to the queue which communicates with
				int receiver_id;
				int id_to_be_sent;
				size_t message_size;
				unsigned char basic_message[MAX_BASIC_MESSAGE_SIZE];
				size_t current_point;
		};

		class extended_IO_task:public basic_IO_task
		{
			public:
				extended_IO_task();
				virtual ~extended_IO_task();

				size_t get_received_data(void* buffer);
				void set_extended_data_size(size_t size);
				size_t get_extended_data_size()const;
				void set_send_buffer(const void* buffer, size_t size);
				virtual void reset();
				const unsigned char* get_send_buffer()const;
				unsigned char* get_receive_buffer(size_t size);
			private:
				size_t extended_size;
				const unsigned char* send_buffer;
				unsigned char* receive_buffer;
		};

		int Send_attr(extended_IO_task* output_task, const struct stat* file_stat);
		int Recv_attr(extended_IO_task* new_task, struct stat* file_stat);
		typedef task_parallel_queue<extended_IO_task> communication_queue_t;
		typedef std::vector<communication_queue_t> communication_queue_array_t;

		class CBB_communication_thread
		{
			public:
				typedef std::map<int, communication_queue_t*> fd_queue_map_t;
			protected:
				CBB_communication_thread()throw(std::runtime_error);
				virtual ~CBB_communication_thread();
				int start_communication_server();
				int stop_communication_server();
				int _add_socket(int socketfd);
				int _add_socket(int socketfd, int op);
				int _delete_socket(int socketfd); 

				virtual int input_from_socket(int socket, communication_queue_array_t* output_queue)=0;
				virtual int input_from_producer(communication_queue_t* input_queue)=0;
				virtual int output_task_enqueue(extended_IO_task* output_task)=0;

				size_t send(extended_IO_task* new_task);
				size_t receive_message(int socket, extended_IO_task* new_task);
				static void* thread_function(void*);
				//thread wait on queue event;
				//static void* queue_wait_function(void*);
				void set_queues(communication_queue_array_t* input_queues,
						communication_queue_array_t* output_queues);
			private:
				//map: socketfd
				typedef std::set<int> socket_pool_t; 
				int set_event_fd()throw(std::runtime_error);

			private:
				int keepAlive;
				int epollfd; 
				socket_pool_t _socket_pool; 

				bool thread_started;
				communication_queue_array_t* input_queue;
				communication_queue_array_t* output_queue;
				pthread_t communication_thread;
				fd_queue_map_t fd_queue_map;
		};


		template<typename T> size_t basic_IO_task::push_back(const T& value)
		{
			return push_backv(value, 1);
		}

		template<typename T> inline size_t basic_IO_task::push_backv(const T& value, size_t num)
		{
			return do_push(&value, num*sizeof(T));
		}

		inline size_t basic_IO_task::do_push(const void* value, size_t num)
		{
			memcpy(basic_message + message_size, value, num);
			message_size += num;
			return num;
		}

		template<typename T> inline size_t basic_IO_task::pop_uncopy(T** var)
		{
			return popv_uncopy(var, 1);
		}

		template<typename T> inline size_t basic_IO_task::popv_uncopy(T** var, size_t num)
		{
			return do_pop(var, num*sizeof(T));
		}

		template<typename T> inline size_t basic_IO_task::do_pop(T** var, size_t num)
		{
			*var=reinterpret_cast<T*>(basic_message + current_point);
			current_point += num;
			return num;
		}

		template<typename T> inline size_t basic_IO_task::pop(T& var)
		{
			return popv(&var, 1); 
		} 

		template<typename T> inline size_t basic_IO_task::popv(T* var, size_t num)
		{
			T* tmp=nullptr;
			size_t ret=popv_uncopy(&tmp, num);
			memcpy(var, tmp, ret);
			return ret;
		}

		inline size_t basic_IO_task::push_back_string(const unsigned char* string)
		{
			return push_back_string(reinterpret_cast<const char*>(string));
		}

		inline size_t basic_IO_task::push_back_string(const char* string)
		{
			size_t len=strlen(string)+1;
			push_back(len);
			do_push(string, len*sizeof(unsigned char));
			return len;
		}

		inline size_t basic_IO_task::push_back_string(const char* string, size_t size)
		{
			push_back(size+1);
			do_push(string, (size+1)*sizeof(unsigned char));
			return size+1;
		}

		inline size_t basic_IO_task::pop_string(unsigned char** var)
		{
			size_t len;
			pop(len);
			do_pop(var, len*sizeof(unsigned char));
			return len;
		}

		inline size_t basic_IO_task::pop_string(char** var)
		{
			return pop_string(reinterpret_cast<unsigned char**>(var));
		}

		inline unsigned char* basic_IO_task::get_message()
		{
			return basic_message;
		}

		inline int basic_IO_task::get_socket()const
		{
			return socket;
		}

		inline void basic_IO_task::reset()
		{
			message_size=0;
			current_point=0;
		}

		inline void basic_IO_task::set_socket(int socket)
		{
			this->socket=socket;
		}

		inline size_t basic_IO_task::get_message_size()const
		{
			return this->message_size;
		}

		inline void basic_IO_task::set_message_size(size_t message_size)
		{
			this->message_size=message_size;
		}

		inline int basic_IO_task::get_receiver_id()const
		{
			return this->receiver_id;
		}

		inline void basic_IO_task::set_receiver_id(int id)
		{
			this->receiver_id=id;
		}

		inline int basic_IO_task::get_id_to_be_sent()const
		{
			return this->id_to_be_sent;
		}

		inline void basic_IO_task::set_id_to_be_sent(int id)
		{
			this->id_to_be_sent=id;
		}

		inline size_t extended_IO_task::get_received_data(void* buffer)
		{
			memcpy(buffer, receive_buffer, extended_size);
			return extended_size;
		}

		inline void extended_IO_task::reset()
		{
			basic_IO_task::reset();
			extended_size = 0;
			send_buffer = nullptr;
		}

		inline size_t extended_IO_task::get_extended_data_size()const
		{
			return extended_size;
		}

		inline void extended_IO_task::set_extended_data_size(size_t size)
		{
			extended_size = size;
		}

		inline void extended_IO_task::set_send_buffer(const void*buffer, size_t size)
		{
			send_buffer = static_cast<const unsigned char*>(buffer);
			extended_size = size;
		}

		inline const unsigned char* extended_IO_task::get_send_buffer()const
		{
			return send_buffer;
		}

		inline unsigned char* extended_IO_task::get_receive_buffer(size_t size)
		{
			if(nullptr == this->receive_buffer)
			{
				this->receive_buffer = new unsigned char[size];
			}
			return this->receive_buffer;
		}
	}
}

#endif
