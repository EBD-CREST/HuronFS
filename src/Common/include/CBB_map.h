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

#ifndef CBB_MAP_H_
#define CBB_MAP_H_

#include <map>
#include <stdexcept>

#include "CBB_rwlock.h"
#include "CBB_internal.h"

namespace CBB
{
	namespace Common
	{
		template<class Key, class Value> class CBB_map:public CBB_rwlock
		{
			public:
				typedef std::map<Key, Value> Maptype_t;
				typedef Key key_type;
				typedef Value mapped_type;
				typedef const Value const_mapped_type;
				typedef std::pair<const Key, Value> value_type;
				typedef typename Maptype_t::const_iterator const_iterator;
				typedef typename Maptype_t::iterator iterator;
				typedef typename Maptype_t::size_type size_type;
			public:
				CBB_map()=default;
				~CBB_map()=default;
				std::pair<iterator, bool> insert(const value_type& value);
				iterator find(const Key& key);
				mapped_type& operator[](const Key& key);
				void erase(iterator position);
				void erase(const Key& key);
				mapped_type& at(const Key& key)throw(std::out_of_range);
				mapped_type& at_retry(const Key& key);
				const_mapped_type& at(const Key& key)const throw(std::out_of_range);
				size_type size()const;
				iterator begin();
				iterator end();
				iterator lower_bound(const Key& key);
				const_iterator lower_bound(const Key& key)const;

				const_iterator begin()const;
				const_iterator end()const;
			private:
				Maptype_t _actual_map;
		};

		template<class Key, class Value> inline std::pair<typename CBB_map<Key, Value>::iterator, bool>CBB_map<Key, Value>::insert(const value_type& value)
		{
			CBB_rwlock::wr_lock();
			std::pair<iterator, bool> ret=_actual_map.insert(value);
			CBB_rwlock::unlock();
			return ret;
		}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::iterator
			CBB_map<Key, Value>::find(const Key& key)
			{
				CBB_rwlock::rd_lock();
				iterator it=_actual_map.find(key);
				CBB_rwlock::unlock();
				return it;
			}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::mapped_type&
			CBB_map<Key, Value>::operator[](const Key& key)
			{
				CBB_rwlock::rd_lock();
				mapped_type& ret=_actual_map[key];
				CBB_rwlock::unlock();
				return ret;
			}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::mapped_type&
			CBB_map<Key, Value>::at_retry(const Key& key)
			{
				CBB_rwlock::rd_lock();
				while(true)
				{
					try
					{
						mapped_type& ret=_actual_map.at(key);
						CBB_rwlock::unlock();
						return ret;
					}
					catch(std::out_of_range)
					{
						_DEBUG("at failed key=%d\n", key);
					}
				}
			}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::mapped_type&
			CBB_map<Key, Value>::at(const Key& key)throw(std::out_of_range)
			{
				CBB_rwlock::rd_lock();
				mapped_type& ret=_actual_map.at(key);
				CBB_rwlock::unlock();
				return ret;
			}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::const_mapped_type&
			CBB_map<Key, Value>::at(const Key& key)const throw(std::out_of_range)
			{
				CBB_rwlock::rd_lock();
				const_mapped_type& ret=_actual_map.at(key);
				CBB_rwlock::unlock();
				return ret;
			}

		template<class Key, class Value> inline void CBB_map<Key, Value>::erase(iterator position)
		{
			CBB_rwlock::wr_lock();
			_actual_map.erase(position);
			CBB_rwlock::unlock();
		}

		template<class Key, class Value> inline void CBB_map<Key, Value>::erase(const Key& key)
		{
			CBB_rwlock::wr_lock();
			_actual_map.erase(key);
			CBB_rwlock::unlock();
		}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::size_type CBB_map<Key, Value>::size()const
		{
			size_type size=_actual_map.size();
			return size;
		}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::iterator CBB_map<Key, Value>::begin()
		{
			return _actual_map.begin();
		}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::const_iterator CBB_map<Key, Value>::begin()const
		{
			return _actual_map.begin();
		}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::iterator CBB_map<Key, Value>::end()
		{
			return _actual_map.end();
		}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::const_iterator CBB_map<Key, Value>::end()const
		{
			return _actual_map.end();
		}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::iterator CBB_map<Key, Value>::lower_bound(const Key& key)
		{
			return _actual_map.lower_bound(key);
		}

		template<class Key, class Value> inline typename CBB_map<Key, Value>::const_iterator CBB_map<Key, Value>::lower_bound(const Key& key)const
		{
			return _actual_map.lower_bound(key);
		}
	}
}

#endif
