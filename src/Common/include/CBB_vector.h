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

#ifndef CBB_VECTOR_H_
#define CBB_VECTOR_H_

#include <map>
#include <stdexcept>

#include "CBB_rwlock.h"

namespace CBB
{
	namespace Common
	{
		template<class type> class CBB_vector:public CBB_rwlock
		{
			public:
				typedef type value_type;
				typedef type& reference;
				typedef const type& const_reference;
				typedef typename std::vector<type>::iterator iterator;
				typedef typename std::vector<type>::const_iterator const_iterator;
				typedef typename std::vector<type>::size_type size_type;

				CBB_vector() = default;
				CBB_vector(size_type size);
				~CBB_vector() = default;

				reference at(size_type pos)throw(std::out_of_range);
				const_reference at(size_type)const throw(std::out_of_range);
				reference operator[](size_type pos);
				const_reference operator[](size_type pos)const;

				iterator begin();
				iterator end();
				bool empty()const;
				size_type size() const;
				void push_back(const type& value);
			private:
				std::vector<type> _actual_vector;
		};

		template<class type> CBB_vector<type>::CBB_vector(size_type size):
			_actual_vector(size)
		{}

		template<class type> inline typename CBB_vector<type>::reference CBB_vector<type>::
			at(size_type pos) throw(std::out_of_range)
		{
			CBB_rwlock::rd_lock();
			reference ret=_actual_vector.at(pos);
			CBB_rwlock::unlock();
		}

		template<class type> inline typename CBB_vector<type>::const_reference CBB_vector<type>::
			at(size_type pos)const throw(std::out_of_range)
		{
			CBB_rwlock::rd_lock();
			const_reference ret=_actual_vector.at(pos);
			CBB_rwlock::unlock();
			return ret;
		}

		template<class type> inline typename CBB_vector<type>::reference CBB_vector<type>::
			operator[](size_type pos)
		{
			CBB_rwlock::rd_lock();
			reference ret=_actual_vector[pos];
			CBB_rwlock::unlock();
			return ret;
		}

		template<class type> inline typename CBB_vector<type>::const_reference CBB_vector<type>::
			operator[](size_type pos)const
		{
			CBB_rwlock::rd_lock();
			const_reference ret=_actual_vector[pos];
			CBB_rwlock::unlock();
			return ret;
		}

		template<class type> inline typename CBB_vector<type>::iterator CBB_vector<type>::begin()
		{
			return _actual_vector.begin();
		}

		template<class type> inline typename CBB_vector<type>::iterator CBB_vector<type>::end()
		{
			return _actual_vector.end();
		}

		template<class type> inline bool CBB_vector<type>::empty()const
		{
			CBB_rwlock::rd_lock();
			bool ret=_actual_vector.empty();
			CBB_rwlock::unlock();
			return ret;
		}

		template<class type> inline typename CBB_vector<type>::size_type CBB_vector<type>::size()const
		{
			CBB_rwlock::rd_lock();
			size_type ret=_actual_vector.size();
			CBB_rwlock::unlock();
			return ret;
		}

		template<class type> inline void CBB_vector<type>::push_back(const type& value)
		{
			CBB_rwlock::wr_lock();
			_actual_vector.push_back(value);
			CBB_rwlock::unlock();
		}
	}
}
#endif
