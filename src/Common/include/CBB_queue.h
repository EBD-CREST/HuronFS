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
