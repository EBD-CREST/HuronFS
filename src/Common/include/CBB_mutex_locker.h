#ifndef CBB_MUTEX_LOCKER_H_
#define CBB_MUTEX_LOCKER_H_

#ifdef MUTLIPLE_THREAD
#include <pthread.h>
#endif

#include "CBB_const.h"


namespace CBB
{
	namespace Common
	{
		class CBB_mutex_locker
		{
			public:
				CBB_mutex_locker();
				virtual ~CBB_mutex_locker();
				int lock();
				int try_lock();
				int unlock();
			private:
#ifdef MUTLIPLE_THREAD
				pthread_mutex_t locker;
#endif
		};

		inline int CBB_mutex_locker::lock()
		{
#ifdef MUTLIPLE_THREAD
			return pthread_mutex_lock(&locker);
#else
			return SUCCESS;
#endif
		}

		inline int CBB_mutex_locker::try_lock()
		{
#ifdef MUTLIPLE_THREAD
			return pthread_mutex_trylock(&locker);
#else
			return SUCCESS;
#endif
		}

		inline int CBB_mutex_locker::unlock()
		{
#ifdef MUTLIPLE_THREAD
			return pthread_mutex_unlock(&locker);
#else
			return SUCCESS;
#endif
		}
	}
}

#endif
