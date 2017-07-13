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

#ifndef CBB_BASIC_SWAP_H_
#define CBB_BASIC_SWAP_H_

#include <vector>

#include "CBB_const.h"
#include "CBB_list.h"

namespace CBB
{
	namespace Common
	{
		template<typename type> class CBB_basic_swap;
		template<typename type> class access_queue;

		template<typename type> class access_page:
			public list
		{
		public:
			access_page(access_queue<type>* my_queue);
			~access_page()=default;
			int reset_myself();
			type* get_data();
			void set_data(type* data);
			access_page* get_next();
			access_page* get_prev();
			access_page* set_next(access_page* next);
			access_page* set_prev(access_page* prev);
		private:
			type* 			data;
			access_queue<type>* 	my_queue;
		};

		template<typename type> class access_queue
		{
		public:
			friend class CBB_basic_swap<type>;
			access_queue();
			~access_queue();
			access_page<type>* enqueue(access_page<type>* page);
			int dequeue();
			int erase(access_page<type>* elem);
			access_page<type>* push(type* chunk);
			access_page<type>* pop();
			access_page<type>* 
				touch(access_page<type>* chunk);
			bool is_empty()const;
			bool is_full()const;
		private:
			void insert(list* insert, list* before);
			void del(list* elem);

			void destroy();
		private:
			/*push to head*/
			access_page<type>* head;
			/*pop from tail*/
			access_page<type>* tail;
			int		   page_count;
		};
						
		template<typename type> class CBB_basic_swap
		{
		protected:
			CBB_basic_swap();
			virtual ~CBB_basic_swap();
			size_t swap_out(ssize_t size);
			virtual size_t free_data(type* data)=0;
			virtual bool need_writeback(type* data)=0;
			virtual size_t writeback(type* data)=0;
			virtual access_page<type>* 
				access(access_page<type>* data)=0;
			virtual access_page<type>* 
				access(type*data)=0;
			int remove_page_from_swap_queue(access_page<type>* page);
			int prepare_writeback_page(
					std::vector<type*>* writeback_queue,
					int count);
		protected:
			access_queue<type> queue;
		};
			
		template<typename type> type* access_page<type>::
			get_data()
		{
			return this->data;
		}			
		
		template<typename type> void access_page<type>::
			set_data(type* data)
		{
			this->data=data;
		}

		template<typename type> int access_page<type>::
			reset_myself()
		{
			return my_queue->erase(this);
		}

		template<typename type> access_page<type>* access_queue<type>::
			touch(access_page<type>* page)
		{
			del(page);

			insert(page, head->get_prev());
			return page;
		}

		//erase the page that has been deleted from the swap queue   
		template<typename type> int  access_queue<type>::
			erase(access_page<type>* page)
		{
			del(page);

			insert(page, tail->get_prev());
			
			_DEBUG("erase a page %p, page count %d\n", page, page_count--);
			return SUCCESS;
		}

		template<typename type> access_page<type>* access_page<type>::
			get_next()
		{
			return static_cast<access_page<type>*>(list::get_next());
		}

		template<typename type> access_page<type>* access_page<type>::
			get_prev()
		{
			return static_cast<access_page<type>*>(list::get_prev());
		}

		template<typename type> access_page<type>* access_page<type>::
			set_next(access_page<type>* next)
		{
			return static_cast<access_page<type>*>(list::set_next(next));
		}
		
		template<typename type> access_page<type>* access_page<type>::
			set_prev(access_page<type>* prev)
		{
			return static_cast<access_page<type>*>(list::set_prev(prev));
		}

		template<typename type> void access_queue<type>::
			insert( list* insert,
				list* before)
		{
			insert->set_next(before->get_next());
			insert->set_prev(before);
			
			before->get_next()->set_prev(insert);
			before->set_next(insert);
		}
		
		template<typename type> void access_queue<type>::
			del(list* elem)
		{
			elem->get_next()->set_prev(elem->get_prev());
			elem->get_prev()->set_next(elem->get_next());
		}

		template<typename type> access_page<type>::
			access_page(access_queue<type>* my_queue):
				//base class
				list(),
				//field
				data(nullptr),
				my_queue(my_queue)
		{}

		template<typename type> access_queue<type>::
			access_queue():
				//fields
				head(nullptr),
				tail(nullptr),
				page_count(0)
		{
			head=new access_page<type>(this);
			tail=new access_page<type>(this);
			head->set_next(tail);
			head->set_prev(tail);
			tail->set_next(head);
			tail->set_prev(head);
		}

		template<typename type> access_page<type>* access_queue<type>::
			pop()
		{
			access_page<type>* ret=tail->get_next();
			if(ret != head)
			{
				tail=ret;
				_DEBUG("remove a page %p, page count=%d\n", ret, page_count--);

				return ret;
			}
			else
			{
				return nullptr;
			}
		}

		template<typename type> access_queue<type>::
			~access_queue()
		{
			this->destroy();
		}

		template<typename type> void access_queue<type>::
			destroy()
		{
			access_page<type>* next=head;
			access_page<type>* tmp=nullptr;
			while(next != tail)
			{
				tmp=next->get_next();	
				delete next;
				next=tmp;
			}
			delete tail;
		}

		template<typename type> CBB_basic_swap<type>::
			CBB_basic_swap():
				//field
				queue()
		{}

		template<typename type> CBB_basic_swap<type>::
			~CBB_basic_swap()
		{}

		template<typename type> bool access_queue<type>::
			is_full()const
		{
			return this->head->get_next() == this->tail;
		}

		template<typename type> bool access_queue<type>::
			is_empty()const
		{
			return this->head == this->tail->get_next();
		}

		template<typename type> access_page<type>* access_queue<type>::
			push(type* data)
		{
			access_page<type>* new_element=nullptr;

			if(is_full())
			{
				new_element=new access_page<type>(this);
				new_element->set_data(data);
				enqueue(new_element);
				_DEBUG("allocate a new element\n");
			}
			else
			{
				new_element=head;
				new_element->set_data(data);
				head=head->get_next();
				_DEBUG("reuse an element\n");
			}

			_DEBUG("track a new page, total pages %d\n", page_count++);

			return new_element;
		}
		
		template<typename type> access_page<type>* access_queue<type>::
			enqueue(access_page<type>* page)
		{
			insert(page, head->get_prev());
			return page;
		}

		/* use count == -1 to say infinite */
		template<typename type> int CBB_basic_swap<type>::
			prepare_writeback_page(
				std::vector<type*>* writeback_queue,
				int count)
		{
			int total=count;

			writeback_queue->clear();

			access_page<type>* tmp=queue.tail->get_next();
			while(count &&
				tmp != queue.head)
			{
				if(need_writeback(tmp->get_data()))
				{
					_LOG("write back page\n");
					writeback_queue->push_back(tmp->get_data());
					--count;
				}
				tmp=tmp->get_next();
			}

			return total-count;
		}

		/* use count == -1 to say infinite */
		template<typename type> size_t CBB_basic_swap<type>::
			swap_out(ssize_t size)
		{
			ssize_t total_size=size;

			access_page<type>* tmp=nullptr;
			while(0 < size &&
				nullptr != (tmp=queue.pop()))
			{
				if(need_writeback(tmp->get_data()))
				{
					_LOG("need write back\n");
					writeback(tmp->get_data());
				}

				size -= free_data(tmp->get_data());
			}

			if(0 != size)
			{
				_LOG("cannot free memory for %ld\n", size);
			}

			return total_size-size;
		}

		template<typename type> int CBB_basic_swap<type>::
			remove_page_from_swap_queue(access_page<type>* page)
		{
			return queue.erase(page);
		}
	}
}

#endif