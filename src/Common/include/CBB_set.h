#ifndef CBB_SET_H_
#define CBB_SET_H_

#include <set>
#include <stdexcept>
#include <pthread.h>

#include "CBB_rwlocker.h"

namespace CBB
{
	namespace Common
	{
		template<class Key> class CBB_set:public CBB_rwlocker
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
				CBB_set();
				~CBB_set();
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
		template<class Key>CBB_set<Key>::CBB_set():
			CBB_rwlocker(),
			_actual_set()
		{}

		template<class Key>CBB_set<Key>::~CBB_set()
		{}

		template<class Key> inline std::pair<typename CBB_set<Key>::iterator, bool>CBB_set<Key>::insert(const value_type& value)
		{
			std::pair<iterator, bool> ret;
			//CBB_rwlocker::wr_lock();
			ret=_actual_set.insert(value);
			//CBB_rwlocker::unlock();
			return ret;
		}

		template<class Key> inline typename CBB_set<Key>::iterator
			CBB_set<Key>::find(const Key& key)
			{
				iterator ret;
				//CBB_rwlocker::wr_lock();
				ret=_actual_set.find(key);
				//CBB_rwlocker::unlock();
				return ret;
			}

		template<class Key> inline void CBB_set<Key>::erase(iterator position)
		{
			//CBB_rwlocker::wr_lock();
			_actual_set.erase(position);
			//CBB_rwlocker::unlock();
		}

		template<class Key> inline void CBB_set<Key>::erase(const Key& key)
		{
			//CBB_rwlocker::wr_lock();
			_actual_set.erase(key);
			//CBB_rwlocker::unlock();
		}

		/*template<class Key> typename CBB_set<Key>::value_type&
		  CBB_set<Key>::at(const Key& key):throw(std::out_of_range)
		  {
		  Key ret;
		  CBB_rwlocker::rd_lock();
		  ret=_actual_set.at(key);
		  CBB_rwlocker::unlock();
		  return ret;
		  }*/

		template<class Key> inline typename CBB_set<Key>::size_type CBB_set<Key>::size()const
		{
			return _actual_set.size();
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
