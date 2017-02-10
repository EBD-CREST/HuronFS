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

#ifndef CBB_QUEUE_H_
#define CBB_QUEUE_H_

#include <queue>
#include <stdexcept>

#include "CBB_rwlock.h"

namespace CBB
{
	namespace Common
	{
		template<class Key> class CBB_queue:public CBB_rwlock
		{
			public:
				typedef std::queue<Key> Queue_t;
				typedef Key key_type;
				typedef Key value_type;
				typedef value_type& reference;
				typedef typename Queue_t::size_type size_type;
			private:
				Queue_t _actual_queue;
			public:
				CBB_queue()=default;
				~CBB_queue()=default;
				void push(const value_type& val);
				void push(value_type& val);
				void pop();
				size_type size()const;
				reference front();
		};

		template<class Key> inline void CBB_queue<Key>::push(const value_type& val)
		{
			CBB_rwlock::wr_lock();
			_actual_queue.push(val);
			CBB_rwlock::unlock();
			return;
		}

		template<class Key> inline void CBB_queue<Key>::push(value_type& val)
		{
			CBB_rwlock::wr_lock();
			_actual_queue.push(val);
			CBB_rwlock::unlock();
			return;
		}

		template<class Key> inline void CBB_queue<Key>::pop()
		{
			CBB_rwlock::wr_lock();
			_actual_queue.pop();
			CBB_rwlock::unlock();
			return ;
		}

		template<class Key> inline typename CBB_queue<Key>::size_type CBB_queue<Key>::size()const
		{
			CBB_rwlock::rd_lock()
			size_type ret=_actual_queue.size();
			CBB_rwlock::unlock()
		}


		template<class Key> inline typename CBB_queue<Key>::reference CBB_queue<Key>::front()
		{
			return _actual_queue.front();
		}
	}
}

#endif
