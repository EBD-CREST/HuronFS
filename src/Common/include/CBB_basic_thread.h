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


#ifndef CBB_BASIC_THREAD_H_
#define CBB_BASIC_THREAD_H_

#include <stdio.h>
#include <pthread.h>

#include "CBB_const.h"

namespace CBB
{
	namespace Common
	{
		class CBB_basic_thread
		{
		public:
			CBB_basic_thread(int thread_number);
			virtual ~CBB_basic_thread();
			int create_thread(void* (*thread_fun)(void*), void* argument);
			virtual int init_thread();
			bool keepAlive()const;
			int end_thread();
			int setaffinity();
		private:
			int		thread_number;
			int 		keepAlive_flag;
			pthread_t 	thread_id;
		};

		inline bool CBB_basic_thread::
			keepAlive()const
		{
			return KEEP_ALIVE == this->keepAlive_flag;
		}
	}
}

#endif
