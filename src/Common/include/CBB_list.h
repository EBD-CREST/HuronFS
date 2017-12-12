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

#ifndef CBB_LIST_H_
#define CBB_LIST_H_

namespace CBB
{
	namespace Common
	{

		class list
		{
			public:
				list();
				virtual ~list()=default;
				virtual list* get_next();
				virtual list* get_prev();
				virtual list* set_next(list* next);
				virtual list* set_prev(list* prev);
				void insert_after(list* insert);

			private:
				list*	next;
				list*	prev;
		};

		inline list::
			list():
				next(nullptr),
				prev(nullptr)
		{}

		inline list* list::
			get_next()
		{
			return this->next;
		}

		inline list* list::
			get_prev()
		{
			return this->prev;
		}

		inline list* list::
			set_next(list* next)
		{
			this->next=next;
			return next;
		}
		
		inline list* list::
			set_prev(list* prev)
		{
			this->prev=prev;
			return next;
		}

		inline void list::
			insert_after(list* elem)
		{
			elem->set_next(this->get_next());
			elem->set_prev(this);
			
			this->get_next()->set_prev(elem);
			this->set_next(elem);
		}
	}
}
#endif
