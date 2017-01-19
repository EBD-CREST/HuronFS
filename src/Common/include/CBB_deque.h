#ifndef CBB_DEQUE_H_
#define CBB_DEQUE_H_

#include "CBB_rwlock.h"
#include <deque>

namespace CBB
{
	namespace Common
	{
		template<class Key> class CBB_deque: public CBB_rwlock
		{
			public:
				typedef std::deque<Key> Deque_t;
				typedef Key key_type;
				typedef Key value_type;
				typedef value_type& reference;
				typedef const value_type& const_reference;
				typedef typename Deque_t::size_type size_type;
			private:
				Deque_t _actual_deque;
			public:
				CBB_deque();
				virtual ~CBB_deque();
				void push_back(const value_type& val);
				void push_front(const value_type& val);
				void pop_back();
				void pop_front();
				size_type size()const;
				reference front();
				reference back();

		};
		template<class Key>CBB_deque<Key>::CBB_deque():
			CBB_rwlock(),
			_actual_deque()
		{}

		template<class Key>CBB_deque<Key>::~CBB_deque()
		{}

		inline template<class Key> void CBB_deque<Key>::push_back(const value_type& val)
		{
			//CBB_rwlock::wr_lock();
			_actual_deque.push_back(val);
			//CBB_rwlock::unlock();
			return;
		}

		inline template<class Key> void CBB_deque<Key>::push_front(const value_type& val)
		{
			//CBB_rwlock::wr_lock();
			_actual_deque.push_front(val);
			//CBB_rwlock::unlock();
			return;
		}

		inline template<class Key> void CBB_deque<Key>::pop_back()
		{
			//CBB_rwlock::wr_lock();
			_actual_deque.pop_back();
			//CBB_rwlock::unlock();
			return;
		}

		inline template<class Key> void CBB_deque<Key>::pop_front()
		{
			//CBB_rwlock::wr_lock();
			_actual_deque.pop_front();
			//CBB_rwlock::unlock();
			return ;
		}

		inline template<class Key> typename CBB_deque<Key>::size_type CBB_deque<Key>::size()const
		{
			return _actual_deque.size();
		}

		inline template<class Key> typename CBB_deque<Key>::reference CBB_deque<Key>::front()
		{
			//CBB_rwlock::wr_lock();
			reference ret=_actual_deque.front();
			//CBB_rwlock::unlock();
			return ret;
		}

		inline template<class Key> typename CBB_deque<Key>::reference CBB_deque<Key>::back()
		{
			//CBB_rwlock::wr_lock();
			reference ret=_actual_deque.back();
			//CBB_rwlock::unlock();
			return ret;
		}
	}
}

#endif
