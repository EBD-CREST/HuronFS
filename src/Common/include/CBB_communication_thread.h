#ifndef CBB_COMMUNICATION_THREAD_H_
#define CBB_COMMUNICATION_THREAD_H_

#include <pthread.h>
#include <set>
#include <vector>
#include <map>

#include "CBB_task_parallel.h"
#include "CBB_profiling.h"
#include "Comm_api.h"

namespace CBB
{
	namespace Common
	{

		class basic_IO_task:public basic_task
		{
			public:
				basic_IO_task();
				basic_IO_task(int id, basic_IO_task* next);
				virtual ~basic_IO_task()=default;
				template<typename T> size_t push_back(const T& value);
				template<typename T> size_t push_backv(const T& value, size_t num);
				size_t push_back_string(const unsigned char* string);
				size_t push_back_string(const char* string);
				size_t push_back_string(const char* string, size_t size);
				size_t push_back_string(const std::string& string);
				template<typename T> size_t pop(T& var);
				template<typename T> size_t popv(T* var, size_t num);
				template<typename T> size_t pop_uncopy(T** var);
				template<typename T> size_t popv_uncopy(T** var, size_t num);
				template<typename T> size_t do_pop(T** value, size_t num);
				size_t do_push(const void* value, size_t num);
				size_t pop_string(unsigned char** var);
				size_t pop_string(char** var);
				virtual void reset()override;
				virtual void set_id(int id)override final;
				const_comm_handle_t get_handle()const;
				comm_handle_t get_handle();
				void set_handle(const_comm_handle_t handle);
				void set_handle(const comm_handle& handle);
				unsigned char* get_message();
				unsigned char* get_basic_message();
				void set_message_size(size_t message_size);
				size_t get_message_size()const;
				int get_receiver_id()const;
				void set_receiver_id(int id);
				int get_sender_id()const;
				int get_error()const;
				void set_error(int error);
				void swap_sender_receiver();
			public:
				comm_handle handle;
				unsigned char message_buffer[MAX_BASIC_MESSAGE_SIZE];//the first few bytes are reserved for message meta
				int error;
				//remote id refers to the queue which communicates with
				int* receiver_id; //offset = 0
				int* sender_id;  //offset = sizeof(int)
				size_t* message_size; //offset = 2*sizeof(int)
				//extended_size
				//total meta size=2*int + 2*size_t
				unsigned char* basic_message;
				size_t current_point;
		};

		struct send_buffer_element
		{
			const char* buffer;
			size_t size;

			send_buffer_element(const char* buffer, size_t size);
		};

		typedef std::vector<send_buffer_element> send_buffer_t;

		class extended_IO_task:public basic_IO_task
		{
			public:
				extended_IO_task();
				extended_IO_task(int id ,extended_IO_task* next);
				virtual ~extended_IO_task();

				size_t get_received_data(void* buffer, size_t size);
				size_t get_received_data_all(void* buffer);
				void set_extended_data_size(size_t size);
				size_t get_extended_data_size()const;
				virtual void reset()override final;
				const send_buffer_t* get_send_buffer()const;
				unsigned char* get_receive_buffer(size_t size);
				void push_send_buffer(const char* buf, size_t size);
			private:

				size_t* extended_size;
				send_buffer_t send_buffer;
				unsigned char* receive_buffer;
				unsigned char* current_receive_buffer_ptr;
		};

		int Send_attr(extended_IO_task* output_task, const struct stat* file_stat);
		int Recv_attr(extended_IO_task* new_task, struct stat* file_stat);
		typedef task_parallel_queue<extended_IO_task> communication_queue_t;
		typedef std::vector<communication_queue_t> communication_queue_array_t;
		//each thread set the handle which it is waiting on
		typedef std::vector<comm_handle_t> threads_handle_map_t;

		class CBB_communication_thread:
			public CBB_communication,
			public CBB_profiling
		{
			public:
				typedef std::map<int, communication_queue_t*> fd_queue_map_t;
			protected:
				CBB_communication_thread()throw(std::runtime_error);
				virtual ~CBB_communication_thread();
				int start_communication_server();
				int stop_communication_server();
				int _add_event_socket(int socket);
				int _add_handle(comm_handle_t handle);
				int _add_handle(comm_handle_t handle, int op);
				int _delete_handle(comm_handle_t handle); 

				virtual int input_from_network(comm_handle_t handle,
						communication_queue_array_t* output_queue)=0;
				virtual int input_from_producer(communication_queue_t* input_queue)=0;
				virtual int output_task_enqueue(extended_IO_task* output_task)=0;
				virtual communication_queue_t* get_communication_queue_from_handle(comm_handle_t handle)=0;

				size_t send(extended_IO_task* new_task)throw(std::runtime_error);
				size_t receive_message(comm_handle_t handle, extended_IO_task* new_task)throw(std::runtime_error);
				static void* sender_thread_function(void*);
				static void* receiver_thread_function(void*);
				//thread wait on queue event;
				//static void* queue_wait_function(void*);
				void setup(communication_queue_array_t* input_queues,
						communication_queue_array_t* output_queues);
			private:

				void set_queues(communication_queue_array_t* input_queues,
						communication_queue_array_t* output_queues);
			private:
				//map: handle
				typedef std::set<comm_handle_t> handle_pool_t; 
				int set_event_fd()throw(std::runtime_error);

			private:
				int keepAlive;
				int sender_epollfd;
				int receiver_epollfd; 
				int server_socket;
				handle_pool_t _handle_pool; 

				bool thread_started;
				communication_queue_array_t* input_queue;
				communication_queue_array_t* output_queue;
				//to solve both-sending deadlock
				//we create a sender and a reciver
				pthread_t sender_thread;
				pthread_t receiver_thread;
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
			memcpy(basic_message + (*message_size), value, num);
			*(this->message_size) += num;
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
			this->current_point += num;
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

		inline size_t basic_IO_task::push_back_string(const std::string& String)
		{
			return push_back_string(String.c_str(), String.size());
		}

		inline size_t basic_IO_task::pop_string(unsigned char** var)
		{
			size_t len=0;
			pop(len);
			do_pop(var, len*sizeof(unsigned char));
			return len;
		}

		inline size_t basic_IO_task::pop_string(char** var)
		{
			return pop_string(reinterpret_cast<unsigned char**>(var));
		}
		
		inline unsigned char* basic_IO_task::get_basic_message()
		{
			return this->message_buffer+MESSAGE_META_OFF;
		}

		inline unsigned char* basic_IO_task::get_message()
		{
			return this->message_buffer;
		}

		inline const_comm_handle_t basic_IO_task::
			get_handle()const
		{
			return &(this->handle);
		}

		inline comm_handle_t basic_IO_task::
			get_handle()
		{
			return &(this->handle);
		}

		inline void basic_IO_task::reset()
		{
			set_message_size(0);
			this->current_point=0;
			this->error=SUCCESS;
		}

		inline void basic_IO_task::set_id(int id)
		{
			basic_task::set_id(id);
			*(this->sender_id)=id;
		}

		inline void basic_IO_task::set_handle(const_comm_handle_t handle)
		{
			memcpy(&(this->handle), handle, sizeof(this->handle));
		}

		inline void basic_IO_task::set_handle(const comm_handle& handle)
		{
			return set_handle(&handle);
		}


		inline size_t basic_IO_task::get_message_size()const
		{
			return *(this->message_size);
		}

		inline void basic_IO_task::set_message_size(size_t message_size)
		{
			*(this->message_size)=message_size;
		}

		inline int basic_IO_task::get_receiver_id()const
		{
			return *(this->receiver_id);
		}

		inline void basic_IO_task::set_receiver_id(int id)
		{
			*(this->receiver_id)=id;
		}

		inline int basic_IO_task::get_error()const
		{
			return this->error;
		}

		inline int basic_IO_task::get_sender_id()const
		{
			return *(this->sender_id);
		}

		inline void basic_IO_task::set_error(int error)
		{
			this->error=error;
		}

		inline void basic_IO_task::swap_sender_receiver()
		{
			*(this->receiver_id)=*(this->sender_id);
			*(this->sender_id)=get_id();
		}

		inline size_t extended_IO_task::get_received_data(void* buffer, size_t size)
		{
			memcpy(buffer, current_receive_buffer_ptr, size);
			current_receive_buffer_ptr+=size;
			*(this->extended_size) -= size;
			return size;
		}

		inline size_t extended_IO_task::get_received_data_all(void* buffer)
		{
			size_t ret=*(this->extended_size);
			_DEBUG("receive size %ld\n", *extended_size);
			memcpy(buffer, current_receive_buffer_ptr, *extended_size);
			*extended_size=0;
			return ret;
		}

		inline void extended_IO_task::reset()
		{
			basic_IO_task::reset();
			*(this->extended_size) = 0;
			this->send_buffer.clear();
			this->current_receive_buffer_ptr=this->receive_buffer;
		}

		inline size_t extended_IO_task::get_extended_data_size()const
		{
			return *this->extended_size;
		}

		inline void extended_IO_task::set_extended_data_size(size_t size)
		{
			*this->extended_size=size;
		}

		inline const send_buffer_t* extended_IO_task::get_send_buffer()const
		{
			return &this->send_buffer;
		}

		inline unsigned char* extended_IO_task::get_receive_buffer(size_t size)
		{
			if(nullptr == this->receive_buffer)
			{
				this->receive_buffer = new unsigned char[size];
			}
			this->current_receive_buffer_ptr=this->receive_buffer;
			return this->receive_buffer;
		}
		inline void extended_IO_task::push_send_buffer(const char* buf,	size_t size)
		{
			send_buffer.push_back(send_buffer_element(buf, size));
			*(this->extended_size)+=size;
		}

		/*inline void communication_thread::set_threads_socket_map(threads_socket_map_t* threads_socket_map)
		{
			this->threads_socket_map=threads_socket_map;
		}*/

		inline void CBB_communication_thread::setup(communication_queue_array_t* input_queues,
				communication_queue_array_t* output_queues)
		{
			set_queues(input_queues, output_queues);
			//set_threads_socket_map(threads_socket_map);
		}
	}
}

#endif
