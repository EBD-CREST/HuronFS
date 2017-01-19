#ifndef CBB_MUTEX_LOCKER_H_
#define CBB_MUTEX_LOCKER_H_

#ifdef MUTLIPLE_THREAD
#include <pthread.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include "CBB_const.h"


namespace CBB
{
	namespace Common
	{
		class CBB_mutex_lock
		{
			public:
				CBB_mutex_lock();
				virtual ~CBB_mutex_lock();
				int lock();
				int try_lock();
				int unlock();
			private:
#ifdef MUTLIPLE_THREAD
				pthread_mutex_t lock;
#endif
		};

		inline int CBB_mutex_lock::lock()
		{
#ifdef MUTLIPLE_THREAD
			return pthread_mutex_lock(&lock);
#else
			return SUCCESS;
#endif
		}

		inline int CBB_mutex_lock::try_lock()
		{
#ifdef MUTLIPLE_THREAD
			return pthread_mutex_trylock(&lock);
#else
			return SUCCESS;
#endif
		}

		inline int CBB_mutex_lock::unlock()
		{
#ifdef MUTLIPLE_THREAD
			return pthread_mutex_unlock(&lock);
#else
			return SUCCESS;
#endif
		}
	}
}

#endif
