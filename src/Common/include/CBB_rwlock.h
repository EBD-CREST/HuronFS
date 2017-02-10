/*
 * Copyright (c) 2017, Tokyo Institute of Technology
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
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
