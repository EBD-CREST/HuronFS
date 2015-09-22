#include <sched.h>

#include "Server.h"
#include "CBB_task_parallel.h"
#include "CBB_const.h"
#include "CBB_request_handler.h"
#include "CBB_communication_thread.h"

#define PORT 9100
using namespace CBB::Common;

class server: public Server
{
	public:
		server();

		void start_server();

		virtual std::string _get_real_path(const char* path)const;
		virtual std::string _get_real_path(const std::string& path)const;
		virtual int _parse_request(IO_task* new_task, task_parallel_queue<IO_task>* output_queue); 
};

server::server():
	Server(PORT)
{
	_init_server();
}


void server::start_server()
{
	Server::start_server();
	while(1)
	{
		sched_yield();
		sleep(100000);
	}
}

std::string server::_get_real_path(const char* path)const
{
	return std::string(path);
}

std::string server::_get_real_path(const std::string& path)const
{
	return std::string();
}


int server::_parse_request(IO_task* new_task, task_parallel_queue<IO_task>* output_queue)
{
	int int_test;
	size_t size_t_test;
	unsigned char* string_test=NULL;
	const char* send_string="this is a string for sending";
	new_task->pop(int_test);
	new_task->pop(size_t_test);
	new_task->pop_string(&string_test);

	printf("int test %d\nsize_t test %lu\nstring test %s\n", int_test, size_t_test, string_test);
	IO_task* output=init_response_task(new_task, output_queue);

	int_test++;
	size_t_test++;

	output->push_back(int_test);
	output->push_back(size_t_test);
	output->push_back_string(send_string);

	output_queue->task_enqueue();
}

int main(void)
{
	server ser;
	ser.start_server();
}
