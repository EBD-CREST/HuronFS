#ifndef CBB_RWLOCKER_H_
#define CBB_RWLOCKER_H_

#include <pthread.h>
#include "CBB_const.h"

namespace CBB
{
	namespace Common
	{
		class CBB_rwlocker
		{
			public:
				CBB_rwlocker();
				virtual ~CBB_rwlocker();
				int rd_lock();
				int tryrd_lock();
				int wr_lock();
				int trywr_lock();
				int unlock();
			private:
#ifdef MUTIPLE_THREAD
				pthread_rwlock_t locker;
#endif
		};

		inline int CBB_rwlocker::rd_lock()
		{
#ifdef MUTIPLE_THREAD
			return pthread_rwlock_rdlock(&(this->locker));
#else
			return SUCCESS;
#endif
		}

		inline int CBB_rwlocker::tryrd_lock()
		{
#ifdef MUTIPLE_THREAD
			return pthread_rwlock_tryrdlock(&(this->locker));
#else
			return SUCCESS;
#endif
		}

		inline int CBB_rwlocker::wr_lock()
		{
#ifdef MUTIPLE_THREAD
			return pthread_rwlock_wrlock(&locker);
#else
			return SUCCESS;
#endif
		}

		inline int CBB_rwlocker::trywr_lock()
		{
#ifdef MUTIPLE_THREAD
			return pthread_rwlock_trywrlock(&locker);
#else
			return SUCCESS;
#endif
		}

		inline int CBB_rwlocker::unlock()
		{
#ifdef MUTIPLE_THREAD
			return pthread_rwlock_unlock(&locker);
#else
			return SUCCESS;
#endif
		}
	}
}
#endif
