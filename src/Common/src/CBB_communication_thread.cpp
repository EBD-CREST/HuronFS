#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <iterator>

#include "CBB_const.h"
#include "CBB_internal.h"
#include "CBB_communication_thread.h"

using namespace CBB::Common;
using namespace std;

basic_IO_task::basic_IO_task():
	basic_task(),
	handle(),
	message_buffer(),
	error(),
	receiver_id(reinterpret_cast<int*>(message_buffer+RECEIVER_ID_OFF)),
	sender_id(reinterpret_cast<int*>(message_buffer+SENDER_ID_OFF)),
	message_size(reinterpret_cast<size_t*>(message_buffer+MESSAGE_SIZE_OFF)),
	basic_message(message_buffer+MESSAGE_META_OFF),
	current_point(0)
{
	set_message_size(0);
	*sender_id=get_id();
}

basic_IO_task::basic_IO_task(int id, basic_IO_task* next):
	basic_task(id, next),
	handle(),
	message_buffer(),
	error(),
	receiver_id(reinterpret_cast<int*>(message_buffer+RECEIVER_ID_OFF)),
	sender_id(reinterpret_cast<int*>(message_buffer+SENDER_ID_OFF)),
	message_size(reinterpret_cast<size_t*>(message_buffer+MESSAGE_SIZE_OFF)),
	basic_message(message_buffer+MESSAGE_META_OFF),
	current_point(0)
{
	set_message_size(0);
	*sender_id=get_id();
}

send_buffer_element::send_buffer_element(const char* buffer, size_t size):
	buffer(buffer),
	size(size)
{}

extended_IO_task::extended_IO_task():
	basic_IO_task(),
	extended_size(reinterpret_cast<size_t*>(get_message()+EXTENDED_SIZE_OFF)),
	send_buffer(),
	receive_buffer(nullptr),
	current_receive_buffer_ptr(nullptr)
{}

extended_IO_task::extended_IO_task(int id, extended_IO_task* next):
	basic_IO_task(id, next),
	extended_size(reinterpret_cast<size_t*>(get_message()+EXTENDED_SIZE_OFF)),
	send_buffer(),
	receive_buffer(nullptr),
	current_receive_buffer_ptr(nullptr)
{}

extended_IO_task::~extended_IO_task()
{
	if(nullptr != receive_buffer)
	{
		delete[] receive_buffer;
	}
}

CBB_communication_thread::CBB_communication_thread()throw(std::runtime_error):
	keepAlive(KEEP_ALIVE),
	sender_epollfd(-1),
	receiver_epollfd(-1),
	_handle_pool(),
	thread_started(UNSTARTED),
	input_queue(nullptr),
	output_queue(nullptr),
	sender_thread(),
	receiver_thread(),
	fd_queue_map()
{}

CBB_communication_thread::~CBB_communication_thread()
{
	for(handle_pool_t::iterator it=_handle_pool.begin(); 
			it!=_handle_pool.end(); ++it)
	{
		try
		{
			//inform each client that server is shutdown
			Send(*it, sizeof(int));
			Send(*it, I_AM_SHUT_DOWN); 
			//close handle
			Close(*it); 
		}
		catch(std::runtime_error& e)
		{
			;//ignore
		}

	}
	stop_communication_server();
	close(sender_epollfd);
	close(receiver_epollfd);
	for(auto const fd:fd_queue_map)
	{
		close(fd.first);
	}
}

int CBB_communication_thread::start_communication_server()
{
	int ret=SUCCESS;

	if(nullptr == input_queue || nullptr == output_queue)
	{
		return FAILURE;
	}

	if(-1 == (sender_epollfd=epoll_create(LENGTH_OF_LISTEN_QUEUE+1)))
	{
		perror("epoll_creation"); 
		throw std::runtime_error("CBB_communication_thread"); 
	}

	if(-1 == (receiver_epollfd=epoll_create(LENGTH_OF_LISTEN_QUEUE+1)))
	{
		perror("epoll_creation"); 
		throw std::runtime_error("CBB_communication_thread"); 
	}

	set_event_fd();

	if(0 == (ret=pthread_create(&sender_thread, nullptr, sender_thread_function, this)))
	{
		thread_started=STARTED;
	}
	if(0 == (ret=pthread_create(&receiver_thread, nullptr, receiver_thread_function, this)))
	{
		thread_started=STARTED;
	}
	else
	{
		perror("pthread_create");
	}
	return ret;
}

int CBB_communication_thread::stop_communication_server()
{
	keepAlive=NOT_KEEP_ALIVE;
	void* ret=nullptr;

	if(STARTED == thread_started)
	{
		pthread_join(receiver_thread, &ret);
		pthread_join(sender_thread, &ret);
		//pthread_join(queue_event_wait_thread, &ret);
		thread_started = UNSTARTED;
	}
	return SUCCESS;
}

int CBB_communication_thread::_add_event_socket(int socket)
{
	struct epoll_event event; 
	memset(&event, 0, sizeof(event));
	event.data.fd=socket; 
	event.events=EPOLLIN; 
	return epoll_ctl(sender_epollfd, EPOLL_CTL_ADD, socket, &event); 
}

int CBB_communication_thread::_add_handle(comm_handle_t handle)
{
	struct epoll_event event; 
	_DEBUG("add handle socket=%d\n", handle->socket);
	memset(&event, 0, sizeof(event));
	event.data.fd=handle->socket; 
	event.events=EPOLLIN; 
	_handle_pool.insert(handle); 
	return epoll_ctl(receiver_epollfd, EPOLL_CTL_ADD, handle->socket, &event); 
}

int CBB_communication_thread::_add_handle(comm_handle_t handle, int op)
{
	struct epoll_event event; 
	_DEBUG("add handle socket=%d\n", handle->socket);
	memset(&event, 0, sizeof(event));
	event.data.fd=handle->socket;
	event.events=EPOLLIN|op; 
	_handle_pool.insert(handle); 
	return epoll_ctl(receiver_epollfd, EPOLL_CTL_ADD, handle->socket, &event); 
}

int CBB_communication_thread::_delete_handle(comm_handle_t handle)
{
	struct epoll_event event; 
	event.data.fd=handle->socket; 
	event.events=EPOLLIN;
	_handle_pool.erase(handle); 
	return epoll_ctl(receiver_epollfd, EPOLL_CTL_DEL, handle->socket, &event); 
}

void* CBB_communication_thread::sender_thread_function(void* args)
{
	CBB_communication_thread* this_obj=static_cast<CBB_communication_thread*>(args);
	struct epoll_event events[LENGTH_OF_LISTEN_QUEUE]; 
	memset(events, 0, sizeof(struct epoll_event)*(LENGTH_OF_LISTEN_QUEUE)); 
	uint64_t queue_notification;
	while(KEEP_ALIVE == this_obj->keepAlive)
	{
		int nfds=epoll_wait(this_obj->sender_epollfd, events, LENGTH_OF_LISTEN_QUEUE, -1); 
		for(int i=0; i<nfds; ++i)
		{
			int socket=events[i].data.fd;
			_DEBUG("socket %d\n", socket);
			fd_queue_map_t::const_iterator it=this_obj->fd_queue_map.find(socket);
			if(this_obj->fd_queue_map.end() != it)
			{
				_DEBUG("task from socket received\n");
				read(socket, &queue_notification, sizeof(uint64_t));
				this_obj->input_from_producer(it->second);
			}
			else
			{
				_DEBUG("error\n");
			}
		}
	}
	return nullptr;
}

void* CBB_communication_thread::receiver_thread_function(void* args)
{
#ifdef TCP
	CBB_communication_thread* this_obj=static_cast<CBB_communication_thread*>(args);
	struct epoll_event events[LENGTH_OF_LISTEN_QUEUE]; 
	memset(events, 0, sizeof(struct epoll_event)*(LENGTH_OF_LISTEN_QUEUE)); 
	comm_handle handle;

	while(KEEP_ALIVE == this_obj->keepAlive)
	{
		int nfds=epoll_wait(this_obj->receiver_epollfd, events, LENGTH_OF_LISTEN_QUEUE, -1); 
		for(int i=0; i<nfds; ++i)
		{
				handle.socket=events[i].data.fd;
				_DEBUG("task from handle received\n");
				_DEBUG("socket %d\n", handle.socket);
				this_obj->input_from_network(&handle, this_obj->output_queue);
		}
	}
#endif
	return nullptr;
}

size_t CBB_communication_thread::send(extended_IO_task* new_task)throw(std::runtime_error)
{
	size_t ret=0;
	comm_handle_t handle=new_task->get_handle();
	_DEBUG("send message size=%ld, element=%p\n", new_task->get_message_size(), new_task);
	_DEBUG("send message to id =%d from %d\n", new_task->get_receiver_id(), new_task->get_sender_id());

	if(0 != new_task->get_extended_data_size())
	{
		ret+=Sendv(handle,
				new_task->get_message(),
				new_task->get_message_size()+
				MESSAGE_META_OFF);
		_DEBUG("send extended message size=%ld\n", new_task->get_extended_data_size());
		const send_buffer_t* send_buffer=new_task->get_send_buffer();
		for(auto& buf:*send_buffer)
		{
			_DEBUG("send size=%ld\n", buf.size);
			ret+=Send_large(handle, buf.buffer, buf.size);
		}
	}
	else
	{
		ret+=Sendv(handle,
				new_task->get_message(),
				new_task->get_message_size()+
				MESSAGE_META_OFF);
	}
	return ret;
}

size_t CBB_communication_thread::receive_message(comm_handle_t handle, extended_IO_task* new_task)throw(std::runtime_error)
{
	size_t ret=0;

	ret=Recvv_pre_alloc(handle, new_task->get_message()+SENDER_ID_OFF, RECV_MESSAGE_OFF); 
	new_task->swap_sender_receiver();//swap sender receiver id
	_DEBUG("receive basic message size=%ld to %d element=%p extended_size=%ld\n", new_task->get_message_size(),
			new_task->get_receiver_id(), new_task, new_task->get_extended_data_size());
	ret+=Recvv_pre_alloc(handle, new_task->get_basic_message(), new_task->get_message_size());

	new_task->set_handle(handle);

	if(0 != new_task->get_extended_data_size())
	{
		size_t tmp_size=Recv_large(handle,
				new_task->get_receive_buffer(MAX_TRANSFER_SIZE),
				new_task->get_extended_data_size());
		_DEBUG("receive extended message size=%ld, ret=%ld\n", 
				new_task->get_extended_data_size(), tmp_size);
		ret+=tmp_size;
	}
	return ret;
}

void CBB_communication_thread::set_queues(communication_queue_array_t* input_queues,
		communication_queue_array_t* output_queues)
{
	this->input_queue=input_queues;
	this->output_queue=output_queues;
}

int CBB_communication_thread::set_event_fd()throw(std::runtime_error)
{
	for(auto &queue: *input_queue)
	{
		int queue_event_fd=0;
		if(-1 == (queue_event_fd=eventfd(0,0)))
		{
			perror("eventfd");
			throw std::runtime_error("CBB_communication_thread"); 
		}
		queue.set_queue_event_fd(queue_event_fd);
		_add_event_socket(queue_event_fd);
		fd_queue_map.insert(make_pair(queue_event_fd, &queue));
	}
	return SUCCESS;
}

int CBB::Common::Send_attr(extended_IO_task* output_task, const struct stat* file_stat)
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

int CBB::Common::Recv_attr(extended_IO_task* new_task, struct stat* file_stat)
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
