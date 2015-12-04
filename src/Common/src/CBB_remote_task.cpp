#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "CBB_remote_task.h"
#include "CBB_const.h"

using namespace CBB::Common;

remote_task::remote_task():
	mode(0),
	file_stat(nullptr)
{}

remote_task::~remote_task()
{}

CBB_remote_task::CBB_remote_task():
	keepAlive(KEEP_ALIVE),
	thread_started(UNSTARTED),
	remote_task_queue(),
	remote_task_thread(),
	locker()
{}

CBB_remote_task::~CBB_remote_task()
{
	stop_listening();
}

int CBB_remote_task::start_listening()
{
	int ret=0;
	if(0 == (ret=pthread_create(&remote_task_thread, nullptr, thread_fun, this)))
	{
		thread_started=STARTED;
	}
	else
	{
		perror("pthread_create");
	}
	return ret;
}

void CBB_remote_task::stop_listening()
{
	keepAlive=NOT_KEEP_ALIVE;
	void* ret=nullptr;
	if(STARTED == thread_started)
	{
		pthread_join(remote_task_thread, &ret);
		//pthread_join(queue_event_wait_thread, &ret);
		thread_started = UNSTARTED;
	}
	return;
}

void* CBB_remote_task::thread_fun(void* args)
{
	CBB_remote_task* this_obj=static_cast<CBB_remote_task*>(args);

	while(KEEP_ALIVE == this_obj->keepAlive)
	{
		remote_task* new_task=this_obj->remote_task_queue.get_task();
		_DEBUG("remote task received\n");
		this_obj->remote_task_handler(new_task);
		this_obj->remote_task_queue.task_dequeue();
	}
	_DEBUG("request handler thread end\n");
	return nullptr;
}

remote_task* CBB_remote_task::add_remote_task(int task_code, void* file)
{
	remote_task* new_task = remote_task_queue.allocate_tmp_node();
	new_task->set_mode(task_code);
	new_task->set_file_stat(file);
	remote_task_queue.task_enqueue_signal_notification();
	return new_task;
}
