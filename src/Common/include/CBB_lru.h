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

/* this file is for writing back operations in IOnode or Master*/

#ifndef CBB_LRU_H_
#define CBB_LRU_H_

#include "CBB_basic_swap.h"

namespace CBB
{
	namespace Common
	{
		template<typename type> class CBB_lru:
			public CBB_basic_swap<type>
		{
		public:
			CBB_lru()=default;
			virtual ~CBB_lru()=default;

			virtual access_page<type>* 
				access(access_page<type>* data, bool dirty)override final;
			virtual access_page<type>* 
				access(type*data, bool dirty)override final;
			virtual size_t free_data(type* data)=0;
			virtual bool need_writeback(type* data)=0;
			virtual size_t writeback(type* data, const char* mode)=0;
			
		};

		template<typename type> access_page<type>* CBB_lru<type>::
			access(access_page<type>* page, bool dirty)
		{
			return this->queue.touch(page, dirty);
		}

		template<typename type> access_page<type>* CBB_lru<type>::
			access(type* data, bool dirty)
		{
			return this->queue.push(data, dirty);
		}
	}
}
#endif
