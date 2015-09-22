#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "CBB_communication_thread.h"
#include "CBB_const.h"
#include "CBB_internal.h"

using namespace CBB::Common;

CBB_communication_thread::CBB_communication_thread()throw(std::runtime_error):
	keepAlive(KEEP_ALIVE),
	epollfd(epoll_create(LENGTH_OF_LISTEN_QUEUE+1)),
	_socket_pool(),
	thread_started(UNSTARTED),
	input_queue(),
	output_queue(),
	communication_thread(),
	queue_event_fd(eventfd(0, 0))
{
	if(-1  ==  epollfd)
	{
		perror("epoll_creation"); 
		throw std::runtime_error("epoll_creation"); 
	}

	if(-1 == queue_event_fd)
	{
		perror("eventfd");
		throw std::runtime_error("eventfd"); 
	}
}

/*thread_args::thread_args(int (*function)(void*),
		void *args):
	function(function),
	args(args)
{}

thread_args::thread_args():
	function(NULL),
	args(NULL)
{}*/

CBB_communication_thread::~CBB_communication_thread()
{
	stop_communication_server();
	close(epollfd);
	for(socket_pool_t::iterator it=_socket_pool.begin(); 
			it!=_socket_pool.end(); ++it)
	{
		//inform each client that server is shutdown
		Send(*it, sizeof(int));
		Send_flush(*it, I_AM_SHUT_DOWN); 
		//close socket
		close(*it); 
	}
}

int CBB_communication_thread::start_communication_server()
{
	int ret=SUCCESS;
	if(NULL == input_queue || NULL == output_queue)
	{
		return FAILURE;
	}
	input_queue->set_queue_event_fd(queue_event_fd);
	struct epoll_event event; 
	event.data.fd=queue_event_fd; 
	event.events=EPOLLIN|EPOLLPRI; 
	epoll_ctl(epollfd, EPOLL_CTL_ADD, queue_event_fd, &event); 
	/*if(0 == (ret=pthread_create(&communication_thread, NULL, queue_wait_function, this)))
	{
		thread_started=STARTED;
	}*/

	if(0 == (ret=pthread_create(&communication_thread, NULL, thread_function, this)))
	{
		thread_started=STARTED;
	}
	return ret;
}

int CBB_communication_thread::stop_communication_server()
{
	keepAlive=NOT_KEEP_ALIVE;
	void* ret=NULL;
	if(STARTED == thread_started)
	{
		pthread_join(communication_thread, &ret);
		//pthread_join(queue_event_wait_thread, &ret);
		thread_started = UNSTARTED;
	}
	return SUCCESS;
}

int CBB_communication_thread::_add_socket(int socketfd)
{
	struct epoll_event event; 
	event.data.fd=socketfd; 
	//event.events=EPOLLIN|EPOLLET; 
	event.events=EPOLLIN; 
	_socket_pool.insert(socketfd); 
	return epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event); 
}

int CBB_communication_thread::_add_socket(int socketfd, int op)
{
	struct epoll_event event; 
	event.data.fd=socketfd; 
	//event.events=EPOLLIN|EPOLLET; 
	event.events=EPOLLIN|op; 
	_socket_pool.insert(socketfd); 
	return epoll_ctl(epollfd, EPOLL_CTL_ADD, socketfd, &event); 
}

int CBB_communication_thread::_delete_socket(int socketfd)
{
	struct epoll_event event; 
	event.data.fd=socketfd; 
	//event.events=EPOLLIN|EPOLLET;
	event.events=EPOLLIN;
	_socket_pool.erase(socketfd); 
	return epoll_ctl(epollfd, EPOLL_CTL_DEL, socketfd, &event); 
}

/*void* CBB_communication_thread::thread_function(void* args)
{
	thread_args* arg_p=static_cast<thread_args*>(args);
	//pthread_barrier_wait(arg_p->barrier);
	arg_p->function(arg_p->args);
	_DEBUG("communication thread end\n");
	return NULL;
}*/

/*void* CBB_communication_thread::queue_wait_function(void* args)
{
	CBB_communication_thread* this_obj=static_cast<CBB_communication_thread*>(args);
	uint64_t notification=1;
	while(KEEP_ALIVE == this_obj->keepAlive)
	{
		this_obj->input_queue->task_wait();
		//send notification
		write(this_obj->queue_event_fd, &notification, sizeof(uint64_t));
		_DEBUG("input queue task enqueue\n");
	}
	return NULL;
}*/

void* CBB_communication_thread::thread_function(void* args)
{
	CBB_communication_thread* this_obj=static_cast<CBB_communication_thread*>(args);
	struct epoll_event events[LENGTH_OF_LISTEN_QUEUE]; 
	memset(events, 0, sizeof(struct epoll_event)*(LENGTH_OF_LISTEN_QUEUE)); 
	uint64_t queue_notification;
	while(KEEP_ALIVE == this_obj->keepAlive)
	{
		int nfds=epoll_wait(this_obj->epollfd, events, LENGTH_OF_LISTEN_QUEUE, -1); 
		for(int i=0; i<nfds; ++i)
		{
			int socket=events[i].data.fd;
			if(this_obj->queue_event_fd == socket)
			{
				_DEBUG("task from handler received\n");
				read(socket, &queue_notification, sizeof(uint64_t));
				this_obj->input_from_producer(this_obj->input_queue);
			}
			else
			{
				_DEBUG("task from socket received\n");
				this_obj->input_from_socket(socket, this_obj->output_queue);
			}
		}
	}
	return NULL;
}

size_t CBB_communication_thread::send(IO_task* new_task)
{
	size_t ret=0;
	if(0 != new_task->get_extended_message_size())
	{
		_DEBUG("send message size=%ld\n", new_task->get_message_size());
		Send(new_task->get_socket(), new_task->get_message_size());
		//fwrite(new_task->get_message(), new_task->get_message_size(), sizeof(char), stderr);
		putchar('\n');
		_DEBUG("send extended message size=%ld\n", new_task->get_extended_message_size());
		Send(new_task->get_socket(), new_task->get_extended_message_size());
		ret+=Sendv_flush(new_task->get_socket(), new_task->get_extended_message(), new_task->get_extended_message_size());
	}
	else
	{
		_DEBUG("send message size=%ld\n", new_task->get_message_size());
		Send(new_task->get_socket(), new_task->get_message_size());
		ret+=Sendv_flush(new_task->get_socket(), new_task->get_message(), new_task->get_message_size());
		//fwrite(new_task->get_message(), new_task->get_message_size(), sizeof(char), stderr);
		putchar('\n');
	}
	return ret;
}

void CBB_communication_thread::set_queue(task_parallel_queue<IO_task>* input_queue, task_parallel_queue<IO_task>* output_queue)
{
	this->input_queue=input_queue;
	this->output_queue=output_queue;
}

int CBB::Common::Send_attr(IO_task* output_task, const struct stat* file_stat)
{
	output_task->push_back(file_stat->st_mode);    /* protection */
	output_task->push_back(file_stat->st_uid);     /* user ID of owner */
	output_task->push_back(file_stat->st_gid);     /* group ID of owner */
	output_task->push_back(file_stat->st_size);    /* total size, in bytes */
	output_task->push_back(file_stat->st_atime);   /* time of last access */
	output_task->push_back(file_stat->st_mtime);   /* time of last modification */
	output_task->push_back(file_stat->st_ctime);   /* time of last status change */
	return SUCCESS;
}

int CBB::Common::Recv_attr(IO_task* new_task, struct stat* file_stat)
{
	new_task->pop(file_stat->st_mode);    /* protection */
	new_task->pop(file_stat->st_uid);     /* user ID of owner */
	new_task->pop(file_stat->st_gid);     /* group ID of owner */
	new_task->pop(file_stat->st_size);    /* total size, in bytes */
	new_task->pop(file_stat->st_atime);   /* time of last access */
	new_task->pop(file_stat->st_mtime);   /* time of last modification */
	new_task->pop(file_stat->st_ctime);   /* time of last status change */
	return SUCCESS;
}
