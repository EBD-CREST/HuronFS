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

#ifndef CBB_SET_H_
#define CBB_SET_H_

#include <set>
#include <stdexcept>
#include <pthread.h>

#include "CBB_rwlock.h"

namespace CBB
{
	namespace Common
	{
		template<class Key> class CBB_set:public CBB_rwlock
		{
			public:
				typedef std::set<Key> Set_t;
				typedef Key key_type;
				typedef Key value_type;
				typedef typename Set_t::const_iterator const_iterator;
				typedef typename Set_t::iterator iterator;
				typedef typename Set_t::size_type size_type;
			private:
				Set_t _actual_set;
			public:
				CBB_set()=default;
				~CBB_set()=default;
				std::pair<iterator, bool> insert(const value_type& value);
				iterator find(const Key& key);
				void erase(iterator position);
				void erase(const Key& key);
				size_type size()const;
				//value_type& at(const Key& key)throw(std::out_of_range);
				iterator begin();
				iterator end();
				const_iterator begin()const;
				const_iterator end()const;
		};

		template<class Key> inline std::pair<typename CBB_set<Key>::iterator, bool>CBB_set<Key>::insert(const value_type& value)
		{
			CBB_rwlock::wr_lock();
			std::pair<iterator, bool> ret=_actual_set.insert(value);
			CBB_rwlock::unlock();
			return ret;
		}

		template<class Key> inline typename CBB_set<Key>::iterator
			CBB_set<Key>::find(const Key& key)
			{
				CBB_rwlock::wr_lock();
				iterator ret=_actual_set.find(key);
				CBB_rwlock::unlock();
				return ret;
			}

		template<class Key> inline void CBB_set<Key>::erase(iterator position)
		{
			CBB_rwlock::wr_lock();
			_actual_set.erase(position);
			CBB_rwlock::unlock();
		}

		template<class Key> inline void CBB_set<Key>::erase(const Key& key)
		{
			CBB_rwlock::wr_lock();
			_actual_set.erase(key);
			CBB_rwlock::unlock();
		}

		template<class Key> inline typename CBB_set<Key>::size_type CBB_set<Key>::size()const
		{
			CBB_rwlock::rd_lock();
			size_type ret=_actual_set.size();
			CBB_rwlock::unlock();
			return ret;
		}

		template<class Key> inline typename CBB_set<Key>::iterator CBB_set<Key>::begin()
		{
			return _actual_set.begin();
		}

		template<class Key> inline typename CBB_set<Key>::const_iterator CBB_set<Key>::begin()const
		{
			return _actual_set.begin();
		}

		template<class Key> inline typename CBB_set<Key>::iterator CBB_set<Key>::end()
		{
			return _actual_set.end();
		}

		template<class Key> inline typename CBB_set<Key>::const_iterator CBB_set<Key>::end()const
		{
			return _actual_set.end();
		}
	}
}
#endif
