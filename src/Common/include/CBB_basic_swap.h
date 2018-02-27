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
			virtual ~access_page()=default;
			int reset_myself();
			type* get_data();
			void set_data(type* data);
			virtual access_page<type>* get_next()override final;
			virtual access_page<type>* get_prev()override final;
			access_page<type>* set_next(access_page<type>* next);
			access_page<type>* set_prev(access_page<type>* prev);

			access_page* get_next_dirty_page();
			access_page* get_pre_dirty_page();
			void set_next_dirty_page(access_page<type>* next);
			void set_pre_dirty_page(access_page<type>* pre);

			void remove_from_dirty_queue();
			bool is_in_dirty_queue()const;
		private:
			type* 			data;
			access_queue<type>* 	my_queue;
			
			access_page<type>*	next_dirty_page;
			access_page<type>*	pre_dirty_page;
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
			access_page<type>* push(type* chunk, bool dirty);
			access_page<type>* pop();
			access_page<type>* 
				touch(access_page<type>* chunk, bool dirty);
			access_page<type>* get_first_dirty_page();
			bool is_last_dirty_page(access_page<type>* elem)const;
			void del_dirty_page(access_page<type>* elem);
			
			bool is_empty()const;
			bool is_full()const;
		private:
			void del(access_page<type>* elem);

			void destroy();
			void push_dirty_page(access_page<type>* elem);
			void dump_dirty()const;
		private:
			/*push to head*/
			access_page<type>* head;
			/*pop from tail*/
			access_page<type>* tail;
			
			/*the first dirty page*/
			access_page<type>* first_dirty_page;
			/*the last dirty page*/
			access_page<type>* last_dirty_page;

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
			virtual size_t writeback(type* data, const char* mode)=0;
			virtual access_page<type>* 
				access(access_page<type>* data, bool dirty)=0;
			virtual access_page<type>* 
				access(type*data, bool dirty)=0;
			int remove_page_from_swap_queue(access_page<type>* page);

			int prepare_writeback_page(
					std::vector<type*>* writeback_queue,
					int count);

			int check_writeback_page(
					std::vector<type*>& writeback_queue);
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

		template<typename type> access_page<type>* access_page<type>::
			get_next_dirty_page()
		{
			return this->next_dirty_page;
		}

		template<typename type> access_page<type>* access_page<type>::
			get_pre_dirty_page()
		{
			return this->pre_dirty_page;
		}

		template<typename type> void access_page<type>::
			set_next_dirty_page(access_page<type>* next)
		{
			this->next_dirty_page=next;
		}

		template<typename type> void access_page<type>::
			set_pre_dirty_page(access_page<type>* pre)
		{
			this->pre_dirty_page=pre;
		}
			
		template<typename type> access_page<type>* access_queue<type>::
			touch(access_page<type>* page, bool dirty)
		{
			bool dirty_already=page->is_in_dirty_queue();
			if(dirty_already)
			{
				_DEBUG("dirty already\n");
			}

			del(page);
			head->get_prev()->insert_after(page);

			if(dirty||dirty_already)
			{
				push_dirty_page(page);
			}
			return page;
		}

		template<typename type> access_page<type>* access_queue<type>::
			get_first_dirty_page()
		{
			return this->first_dirty_page->get_next_dirty_page();
		}

		template<typename type> bool access_queue<type>::
			is_last_dirty_page(access_page<type>* elem)const
		{
			return elem == this->last_dirty_page;
		}

		//erase the page that has been deleted from the swap queue   
		template<typename type> int  access_queue<type>::
			erase(access_page<type>* page)
		{
			del(page);

			tail->get_prev()->insert_after(page);
			
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
			del(access_page<type>* elem)
		{
			elem->get_next()->set_prev(elem->get_prev());
			elem->get_prev()->set_next(elem->get_next());
			del_dirty_page(elem);
		}

		template<typename type> access_page<type>::
			access_page(access_queue<type>* my_queue):
				//base class
				list(),
				//field
				data(nullptr),
				my_queue(my_queue),
				next_dirty_page(nullptr),
				pre_dirty_page(nullptr)
		{}

		template<typename type> bool access_page<type>::
			is_in_dirty_queue()const
		{
			return nullptr != this->next_dirty_page;
		}

		template<typename type> access_queue<type>::
			access_queue():
				//fields
				head(new access_page<type>(this)),
				tail(new access_page<type>(this)),
				first_dirty_page(new access_page<type>(this)),
				last_dirty_page(first_dirty_page),
				page_count(0)
		{
			first_dirty_page->set_next_dirty_page(last_dirty_page);
			last_dirty_page->set_pre_dirty_page(first_dirty_page);

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

		template<typename type> void access_queue<type>::
			push_dirty_page(access_page<type>* elem)
		{
			if(!elem->is_in_dirty_queue())
			{
				_DEBUG("push page %p\n", elem);
				elem->set_pre_dirty_page(this->last_dirty_page->get_pre_dirty_page());
				elem->set_next_dirty_page(this->last_dirty_page);

				this->last_dirty_page->get_pre_dirty_page()->set_next_dirty_page(elem);
				this->last_dirty_page->set_pre_dirty_page(elem);
				//dump_dirty();
			}
		}

		template<typename type> void access_queue<type>::
			dump_dirty()const
		{
			access_page<type>* tmp=this->first_dirty_page->get_next_dirty_page();
			while(tmp!=this->last_dirty_page)
			{
				_DEBUG("tmp = %p\n", tmp);
				if(tmp->get_next_dirty_page()->get_pre_dirty_page()!=tmp)
				{
					_DEBUG("ERROR!!\n");
				}
				tmp=tmp->get_next_dirty_page();
			}
		}

		template<typename type> void access_queue<type>::
			del_dirty_page(access_page<type>* elem)
		{
			if(elem->is_in_dirty_queue())
			{
				_DEBUG("remove page %p\n", elem);
				elem->get_pre_dirty_page()->set_next_dirty_page(elem->get_next_dirty_page());
				elem->get_next_dirty_page()->set_pre_dirty_page(elem->get_pre_dirty_page());

				elem->set_next_dirty_page(nullptr);
				elem->set_pre_dirty_page(nullptr);

				//dump_dirty();
			}
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
			push(type* data, bool dirty)
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
			
			if(dirty)
			{
				push_dirty_page(new_element);
			}

			_DEBUG("track a new page, total pages %d\n", page_count++);

			return new_element;
		}
		
		template<typename type> access_page<type>* access_queue<type>::
			enqueue(access_page<type>* page)
		{
			head->get_prev()->insert_after(page);

			return page;
		}

		/* use count == -1 to say infinite */
		template<typename type> int CBB_basic_swap<type>::
			prepare_writeback_page(
				std::vector<type*>* writeback_queue,
				int count)
		{
			int total=count;
			int iteration=0;

			writeback_queue->clear();

			/*need to improve the performance here*/
			access_page<type>* tmp=queue.get_first_dirty_page();

			while(count &&
				!queue.is_last_dirty_page(tmp))
			{
				if(need_writeback(tmp->get_data()))
				{
					_LOG("write back page\n");
					_LOG("iteration counts %d to find the %d\n", iteration, total-count+1);
					writeback_queue->push_back(tmp->get_data());
					--count;
				}
				tmp=tmp->get_next_dirty_page();
				iteration++;
			}

			return total-count;
		}

		/* use count == -1 to say infinite */
		template<typename type> int CBB_basic_swap<type>::
			check_writeback_page(
				std::vector<type*>& writeback_queue)
		{
			int count=0;
			/*need to improve the performance here*/
			//access_page<type>* tmp=queue.tail->get_next();
			for(auto item : writeback_queue)
			{
				auto page=item->writeback_page;
				if(!need_writeback(page->get_data()))
				{
					queue.del_dirty_page(page);
					count++;
				}
			}

			return count;
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
				_LOG("swap out page\n");
				if(need_writeback(tmp->get_data()))
				{
					_LOG("need write back\n");
					writeback(tmp->get_data(), "on demand");
					queue.del_dirty_page(tmp);
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
