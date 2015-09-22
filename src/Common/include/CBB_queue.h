#ifndef CBB_QUEUE_H_
#define CBB_QUEUE_H_

#include <queue>
#include <stdexcept>

#include "CBB_mutex_locker.h"

namespace CBB
{
	namespace Common
	{
		template<class Key> class CBB_queue:public CBB_mutex_locker
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
				CBB_queue();
				~CBB_queue();
				void push(const value_type& val);
				void push(value_type& val);
				void pop();
				size_type size()const;
				reference front();
		};

		template<class Key>CBB_queue<Key>::CBB_queue():
			CBB_mutex_locker(),
			_actual_queue()
		{}

		template<class Key>CBB_queue<Key>::~CBB_queue()
		{}

		inline template<class Key> void CBB_queue<Key>::push(const value_type& val)
		{
			//CBB_mutex_locker::lock();
			_actual_queue.push(val);
			//CBB_mutex_locker::unlock();
			return;
		}

		inline template<class Key> void CBB_queue<Key>::push(value_type& val)
		{
			//CBB_mutex_locker::lock();
			_actual_queue.push(val);
			//CBB_mutex_locker::unlock();
			return;
		}

		inline template<class Key> void CBB_queue<Key>::pop()
		{
			//CBB_mutex_locker::lock();
			_actual_queue.pop();
			//CBB_mutex_locker::unlock();
			return ;
		}

		inline template<class Key> typename CBB_queue<Key>::size_type CBB_queue<Key>::size()const
		{
			return _actual_queue.size();
		}


		inline template<class Key> typename CBB_queue<Key>::reference CBB_queue<Key>::front()
		{
			//CBB_mutex_locker::lock();
			reference ret=_actual_queue.front();
			//CBB_mutex_locker::unlock();
			return ret;
		}
	}
}

#endif
