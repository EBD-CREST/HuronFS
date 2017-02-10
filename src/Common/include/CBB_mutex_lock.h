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
