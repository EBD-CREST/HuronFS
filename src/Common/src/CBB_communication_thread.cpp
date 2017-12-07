/*
 * Copyright (c) 2017, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. 
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp, LLNL-CODE-722817.
 * All rights reserved. 
 *
 * This file is part of HuronFS.
 *
 * Please also read the file "LICENSE" included in this package for 
 * Our Notice and GNU Lesser General Public License.
 *
 * This program is free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License (as published by the Free Software 
 * Foundation) version 2.1 dated February 1999. 
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
 * General Public License for more details. 
 *
 * You should have received a copy of the GNU Lesser General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <iterator>

#include "CBB_const.h"
#include "CBB_internal.h"
#include "CBB_communication_thread.h"

using namespace CBB::Common;
using namespace std;

CBB_communication_thread::
CBB_communication_thread()throw(std::runtime_error):
    keepAlive(KEEP_ALIVE),
    sender_epollfd(-1),
    receiver_epollfd(-1),
    server_socket(-1),
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
    stop_communication_server();
    close(sender_epollfd);
    close(receiver_epollfd);
    for (auto const fd : fd_queue_map)
    {
        close(fd.first);
    }
}

int CBB_communication_thread::
start_communication_server()
{
    int ret = SUCCESS;

    if (nullptr == input_queue || nullptr == output_queue)
    {
        return FAILURE;
    }

    if (-1 == (sender_epollfd = epoll_create(LENGTH_OF_LISTEN_QUEUE + 1)))
    {
        perror("epoll_creation");
        throw std::runtime_error("CBB_communication_thread");
    }

    if (-1 == (receiver_epollfd = epoll_create(LENGTH_OF_LISTEN_QUEUE + 1)))
    {
        perror("epoll_creation");
        throw std::runtime_error("CBB_communication_thread");
    }

    set_event_fd();


    cpu_set_t cpu_set;

    if (0 == (ret = pthread_create(&sender_thread, nullptr, sender_thread_function, this)))
    {
        thread_started = STARTED;
    }
    if (0 == (ret = pthread_create(&receiver_thread, nullptr, receiver_thread_function, this)))
    {
        thread_started = STARTED;
    }
    else
    {
        perror("pthread_create");
    }
    CPU_ZERO(&cpu_set);
    CPU_SET(CBB_RECEIVER_THREAD_NUMBER, &cpu_set);

    pthread_setaffinity_np(receiver_thread, sizeof(cpu_set), &cpu_set);

    CPU_ZERO(&cpu_set);
    CPU_SET(CBB_SENDER_THREAD_NUMBER, &cpu_set);

    pthread_setaffinity_np(sender_thread, sizeof(cpu_set), &cpu_set);
    return ret;
}

int CBB_communication_thread::stop_communication_server()
{
    keepAlive = NOT_KEEP_ALIVE;
    void *ret = nullptr;

    if (STARTED == thread_started)
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
    event.data.fd = socket;
    event.events = EPOLLIN;
    return epoll_ctl(sender_epollfd, EPOLL_CTL_ADD, socket, &event);
}

int CBB_communication_thread::_add_handle(comm_handle_t handle)
{
#ifdef TCP
    struct epoll_event event;
    _DEBUG("add handle socket=%d\n", handle->socket);
    memset(&event, 0, sizeof(event));
    int* socket=static_cast<int*>(handle->get_raw_handle());
    event.data.fd = *socket;
    event.events = EPOLLIN;
    _handle_pool.insert(handle);
    return epoll_ctl(receiver_epollfd, EPOLL_CTL_ADD, *socket, &event);
#else
    return SUCCESS;
#endif
}

int CBB_communication_thread::_add_handle(comm_handle_t handle, int op)
{
#ifdef TCP
    struct epoll_event event;
    _DEBUG("add handle socket=%d\n", handle->socket);
    memset(&event, 0, sizeof(event));
    int* socket=static_cast<int*>(handle->get_raw_handle());
    event.data.fd = *socket;
    event.events = EPOLLIN | op;
    _handle_pool.insert(handle);
    return epoll_ctl(receiver_epollfd, EPOLL_CTL_ADD, *socket, &event);
#else
    return SUCCESS;
#endif
}

int CBB_communication_thread::_delete_handle(comm_handle_t handle)
{
#ifdef TCP
    struct epoll_event event;
    int* socket=static_cast<int*>(handle->get_raw_handle());
    event.data.fd = *socket;
    event.events = EPOLLIN;
    _handle_pool.erase(handle);
    return epoll_ctl(receiver_epollfd, EPOLL_CTL_DEL, *socket, &event);
#else
    return SUCCESS;
#endif
}

void *CBB_communication_thread::sender_thread_function(void *args)
{
    CBB_communication_thread *this_obj = static_cast<CBB_communication_thread *>(args);
    struct epoll_event events[LENGTH_OF_LISTEN_QUEUE];
    memset(events, 0, sizeof(struct epoll_event) * (LENGTH_OF_LISTEN_QUEUE));
    uint64_t queue_notification;
    while (KEEP_ALIVE == this_obj->keepAlive)
    {
        int nfds = epoll_wait(this_obj->sender_epollfd, events, LENGTH_OF_LISTEN_QUEUE, -1);
        for (int i = 0; i < nfds; ++i)
        {
            int socket = events[i].data.fd;
            _DEBUG("socket %d\n", socket);
            fd_queue_map_t::const_iterator it = this_obj->fd_queue_map.find(socket);
            if (this_obj->fd_queue_map.end() != it)
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

void *CBB_communication_thread::receiver_thread_function(void *args)
{
    CBB_communication_thread *this_obj = static_cast<CBB_communication_thread *>(args);
    comm_handle handle;
#ifdef TCP
    struct epoll_event events[LENGTH_OF_LISTEN_QUEUE];
    memset(events, 0, sizeof(struct epoll_event) * (LENGTH_OF_LISTEN_QUEUE));

    while (KEEP_ALIVE == this_obj->keepAlive)
    {
        int nfds = epoll_wait(this_obj->receiver_epollfd, events, LENGTH_OF_LISTEN_QUEUE, -1);
        for (int i = 0; i < nfds; ++i)
        {
            handle.set_raw_handle(&events[i].data.fd);
            _DEBUG("task from handle received\n");
            _DEBUG("socket %d\n", handle.socket);
            this_obj->input_from_network(&handle, this_obj->output_queue);
        }
    }
#else
    int 		ret	= 0;
    cci_event_t 	*event	= nullptr;
    cci_endpoint_t *endpoint = this_obj->get_endpoint();
    bool		lock	= false;
    size_t 	   	auth_size = 0;
    comm_handle_t  	handle_ptr = nullptr;
    handle_context*	context =nullptr;

    char       authorization[MAX_BASIC_MESSAGE_SIZE ];

    while (KEEP_ALIVE == this_obj->keepAlive)
    {
        ret = cci_get_event(endpoint, &event);
        if (CCI_SUCCESS != ret)
        {
            if (CCI_EAGAIN != ret)
            {
                _DEBUG("cci_get_event() returned %s\n",
                       cci_strerror(endpoint, static_cast<cci_status>(ret)));
            }
            continue;
        }
        switch (event->type)
        {
        case CCI_EVENT_CONNECT_REQUEST:
        {
            _DEBUG("cci_event_connect_request\n");
            if (!lock)
            {
                ret = cci_accept(event, nullptr);
                if (ret != CCI_SUCCESS)
                {
                    _DEBUG("cci_accept() returned %s\n",
                           cci_strerror(endpoint,
                                        static_cast<cci_status>(ret)));
                }
                else
                {
                    auth_size = event->request.data_len;
                    memcpy(authorization, event->request.data_ptr, auth_size);
                    lock = true;
                }
            }
            else
            {
                cci_reject(event);
            }
        }
        break;
        case CCI_EVENT_ACCEPT:
            //ignore
            handle.cci_handle = event->accept.connection;
            handle.buf = authorization;
            handle.size = auth_size;
            this_obj->input_from_network(&handle, this_obj->output_queue);
            lock = false;
            _DEBUG("cci_event_accept %p\n",
                   event->recv.connection);
            break;
        case CCI_EVENT_SEND:
            _DEBUG("cci_send finished\n");

	    if (nullptr != (context = reinterpret_cast<handle_context*>(event->send.context)))
	    {
		    handle.local_rma_handle=context->local_rma_handle;
		    this_obj->deregister_mem(&handle);
		    if(nullptr != context->block_ptr)
		    {
			    this_obj->after_large_transfer(context->block_ptr);
		    }

		    this_obj->putback_communication_context();
            }
            break;
        case CCI_EVENT_RECV:
            _DEBUG("cci_recv finished from %p\n",
                   event->recv.connection);
            handle.cci_handle = event->recv.connection;
            handle.buf = event->recv.ptr;
            handle.size = event->recv.len;
            this_obj->input_from_network(&handle, this_obj->output_queue);
            break;
        case CCI_EVENT_CONNECT:
            _DEBUG("cci_event_connect finished\n");
            handle_ptr = reinterpret_cast<comm_handle_t>(event->connect.context);
            handle_ptr->cci_handle = event->connect.connection;
            break;

        default:
            _DEBUG("ignoring event type %s\n", cci_event_type_str(event->type));
        }
        cci_return_event(event);
    }
#endif
    return nullptr;
}

size_t CBB_communication_thread::
send(extended_IO_task *new_task)
throw(std::runtime_error)
{
    size_t ret = 0;
    comm_handle_t handle = new_task->get_handle();
    _DEBUG("send message size=%ld, element=%p\n", new_task->get_message_size(), new_task);
    _DEBUG("send message to id =%d from %d\n", new_task->get_receiver_id(), new_task->get_sender_id());

    if (0 != new_task->get_extended_data_size())
    {
#ifdef TCP
        //tcp
        ret += handle->Sendv(
                     new_task->get_message(),
                     new_task->get_message_size() +
                     MESSAGE_META_OFF);
        _DEBUG("send extended message size=%ld\n", new_task->get_extended_data_size());
        const send_buffer_t *send_buffer = new_task->get_send_buffer();
        for (auto &buf : *send_buffer)
        {
            _DEBUG("send size=%ld\n", buf.size);
            ret += handle->Send_large(buf.buffer, buf.size);
        }
#else
        //cci
        const send_buffer_t *send_buffer = new_task->get_send_buffer();
        for (auto &buf : *send_buffer)
        {
            _DEBUG("register size=%ld\n", buf.size);
            register_mem(buf.buffer, buf.size, handle, CCI_FLAG_WRITE | CCI_FLAG_READ);

	    new_task->push_back(*(handle->local_rma_handle));
        }
        ret += handle->Sendv(
                     new_task->get_message(),
                     new_task->get_message_size() +
                     MESSAGE_META_OFF);
        _DEBUG("send extended message size=%ld\n", new_task->get_extended_data_size());
#endif

    }
    else
    {
        ret += handle->Sendv(
                     new_task->get_message(),
                     new_task->get_message_size() +
                     MESSAGE_META_OFF);
    }

    return ret;
}

size_t CBB_communication_thread::
send_rma(extended_IO_task *new_task)
throw(std::runtime_error)
{
    size_t ret = 0;

#ifdef CCI
    comm_handle_t handle = new_task->get_handle();
    const send_buffer_t *send_buffer = new_task->get_send_buffer();
    size_t count = 0;
    handle->dump_remote_key();
    handle_context* context_ptr=nullptr;

    switch (new_task->get_mode())
    {
    case RMA_READ:
	_DEBUG("RMA READ\n");
        count = send_buffer->size();

        for (auto &buf : *send_buffer)
        {
            _DEBUG("register size=%ld\n", buf.size);

            register_mem(buf.buffer, buf.size, handle, CCI_FLAG_WRITE | CCI_FLAG_READ);
	    context_ptr=allocate_communication_context(handle, buf.context);

            if (0 != --count)
            {
                handle->Recv_large(nullptr, buf.size, ret, context_ptr);
            }
            else
            {
                _DEBUG("send completion\n");
                handle->Recv_large(new_task->get_message(),
                           new_task->get_total_message_size(),
                           nullptr, buf.size, ret, context_ptr);

            }
            ret += buf.size;
        }

        break;
    case RMA_WRITE:
	_DEBUG("RMA WRITE\n");

        count = send_buffer->size();
        for (auto &buf : *send_buffer)
        {
            _DEBUG("register memory\n");
            //DELETE debug
            register_mem(buf.buffer, buf.size, handle, CCI_FLAG_READ | CCI_FLAG_WRITE);
	    context_ptr=allocate_communication_context(handle, buf.context);

            if (0 != --count)
            {
                handle->Send_large(nullptr, buf.size, ret, context_ptr);
            }
            else
            {
                _DEBUG("send completion, size %ld\n", new_task->get_total_message_size());
                handle->Send_large(new_task->get_message(),
                           new_task->get_total_message_size(),
                           nullptr, buf.size, ret, context_ptr);
            }
            ret += buf.size;
        }

        break;
    }

    end_recording(this, ret, new_task->get_mode());

    /*if(0 != new_task->get_message_size())
    {
    	Sendv(handle, new_task->get_message(),
    		new_task->get_total_message_size());
    }*/

#endif
    return ret;
}

size_t CBB_communication_thread::
receive_message(comm_handle_t 		handle,
                extended_IO_task 	*new_task)
throw(std::runtime_error)
{
    size_t ret = 0;

    ret = handle->Recvv_pre_alloc(new_task->get_message() + SENDER_ID_OFF, RECV_MESSAGE_OFF);
    _DEBUG("receive basic message size=%ld from %d to %d element=%p extended_size=%ld\n", new_task->get_message_size(),
           new_task->get_sender_id(), new_task->get_receiver_id(), new_task, new_task->get_extended_data_size());
    new_task->swap_sender_receiver();//swap sender receiver id
    ret += handle->Recvv_pre_alloc(new_task->get_basic_message(), new_task->get_message_size());

    new_task->set_handle(handle);

#ifdef TCP
    //start_recording(this);

    if (0 != new_task->get_extended_data_size())
    {
        size_t tmp_size = handle->Recv_large(
                                     new_task->get_receive_buffer(MAX_TRANSFER_SIZE),
                                     new_task->get_extended_data_size());
        _DEBUG("receive extended message size=%ld, ret=%ld\n",
               new_task->get_extended_data_size(), tmp_size);
        ret += tmp_size;
    }

    //end_recording(this, 0, WRITE_FILE);

    //this->print_log_debug("w", "receive extended data", 0, ret);

#endif

    return ret;
}

void CBB_communication_thread::set_queues(communication_queue_array_t *input_queues,
        communication_queue_array_t *output_queues)
{
    this->input_queue = input_queues;
    this->output_queue = output_queues;
}

int CBB_communication_thread::set_event_fd()throw(std::runtime_error)
{
    for (auto &queue : *input_queue)
    {
        int queue_event_fd = 0;
        if (-1 == (queue_event_fd = eventfd(0, 0)))
        {
            perror("eventfd");
            throw std::runtime_error("CBB_communication_thread");
        }
        queue.set_queue_event_fd(queue_event_fd);
        _add_event_socket(queue_event_fd);
        fd_queue_map.insert(make_pair(queue_event_fd, &queue));
	_DEBUG("set up fd for queue %p fd %d\n",
			&queue, queue_event_fd);
    }
    return SUCCESS;
}

int CBB::Common::Send_attr(extended_IO_task *output_task, const struct stat *file_stat)
{
    output_task->push_back(file_stat->st_mode);    /* protection */
    output_task->push_back(file_stat->st_uid);     /* user ID of owner */
    output_task->push_back(file_stat->st_gid);     /* group ID of owner */
    output_task->push_back(file_stat->st_size);    /* total size, in bytes */
    output_task->push_back(file_stat->st_atime);   /* time of last access */
    output_task->push_back(file_stat->st_mtime);   /* time of last modification */
    output_task->push_back(file_stat->st_ctime);   /* time of last status change */
    output_task->push_back(file_stat->st_blksize); /* blocksize for file system I/O */
    return SUCCESS;
}

int CBB::Common::Recv_attr(extended_IO_task *new_task, struct stat *file_stat)
{
    memset(file_stat, 0, sizeof(struct stat));
    new_task->pop(file_stat->st_mode);    /* protection */
    new_task->pop(file_stat->st_uid);     /* user ID of owner */
    new_task->pop(file_stat->st_gid);     /* group ID of owner */
    new_task->pop(file_stat->st_size);    /* total size, in bytes */
    new_task->pop(file_stat->st_atime);   /* time of last access */
    new_task->pop(file_stat->st_mtime);   /* time of last modification */
    new_task->pop(file_stat->st_ctime);   /* time of last status change */
    new_task->pop(file_stat->st_blksize); /* blocksize for file system I/O */
    return SUCCESS;
}
