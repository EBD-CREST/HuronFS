#ifndef CBB_RWLOCKER_H_
#define CBB_RWLOCKER_H_

#include <pthread.h>
#include "CBB_const.h"

namespace CBB
{
	namespace Common
	{
		class CBB_rwlock
		{
			public:
				CBB_rwlock();
				virtual ~CBB_rwlock();
				int rd_lock();
				int tryrd_lock();
				int wr_lock();
				int trywr_lock();
				int unlock();
			private:
#ifdef MUTIPLE_THREAD
				pthread_rwlock_t lock;
#endif
		};

		inline int CBB_rwlock::rd_lock()
		{
#ifdef MUTIPLE_THREAD
			return pthread_rwlock_rdlock(&(this->lock));
#else
			return SUCCESS;
#endif
		}

		inline int CBB_rwlock::tryrd_lock()
		{
#ifdef MUTIPLE_THREAD
			return pthread_rwlock_tryrdlock(&(this->lock));
#else
			return SUCCESS;
#endif
		}

		inline int CBB_rwlock::wr_lock()
		{
#ifdef MUTIPLE_THREAD
			return pthread_rwlock_wrlock(&lock);
#else
			return SUCCESS;
#endif
		}

		inline int CBB_rwlock::trywr_lock()
		{
#ifdef MUTIPLE_THREAD
			return pthread_rwlock_trywrlock(&lock);
#else
			return SUCCESS;
#endif
		}

		inline int CBB_rwlock::unlock()
		{
#ifdef MUTIPLE_THREAD
			return pthread_rwlock_unlock(&lock);
#else
			return SUCCESS;
#endif
		}
	}
}
#endif
