#include "Thread_pool.h"

using namespace CBB::Common;
job::job(int sockfd):
	socket(sockfd)
{}

job::job():
	socket()
{}

Thread_pool::Thread_pool(int num_threads):
	num_threads(num_threads),
	workers(worker_pool_t(num_threads)),
	job_queue()
{
	create_thread_pool();
}

Thread_pool::~Thread_pool()
{
	destory_thread_pool();
}

void Thread_pool::create_thread_pool()throw(std::runtime_error)
{
	for(int i=0;i<num_threads;++i)
	{
		pthread_t new_thread;
		if(0 != pthread_create(&new_thread, NULL, (void*) thread_routine, NULL))
		{
			perror("thread create error");
			throw(std::runtime_error("thread create error"));
		}
		workers.push_back(thread(new_thread, i));
	}
}

void* Thread_pool::thread_routine()
{
	while(keepAlive)
	{
		pthread_cond_wait(&queue_empty, &cond_mutex);
		do_work(detach_job());
	}
	pthread_exit(NULL);
}

void Thread_pool::destory_thread_pool()
{
	pthread_cond_wait(&queue_empty, &cond_mutex);
	keepAlive=false;
	for(worker_pool_t::iterator it=workers.begin();
			workers.end()!= it; ++it)
	{
		pthread_join(it->thread_handler, NULL);
	}
	return ;
}

int Thread_pool::add_job(job& new_job)
{
	job_queue.lock();
	if(MAX_THREAD_POOL_WORK_QUEUE_SIZE == job_queue.size())
	{
		return FAILURE;
	}
	job_queue.push(new_job);
	job_queue.unlock();
	return SUCCESS;
}

job Thread_pool::detach_job()
{
	job_queue.lock();
	while(0 == job_queue.size())
	{
		job_queue.unlock();
		pthread_cond_wait(&queue_not_empty, &cond_mutex);
		if(!keepAlive)
		{
			pthread_exit(NULL);
		}
		job_queue.lock();
	}
	job new_job=job_queue.front();
	job_queue.pop();
	if(0 == job_queue.size())
	{
		pthread_mutex_lock(&cond_mutex);
		pthread_cond_signal(&queue_empty);
		pthread_mutex_unlock(&cond_mutex);
	}
	job_queue.unlock();
	return job;
}
