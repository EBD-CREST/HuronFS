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

#ifndef IONODE_BASIC_H_
#define IONODE_BASIC_H_

#include <map>
#include <list>

#include "Comm_api.h"
#include "CBB_swap.h"
#include "CBB_memory_pool.h"

namespace CBB
{
	namespace IOnode
	{
		struct file;
		struct block
		{
			block(off64_t 	start_point,
				size_t 	block_size,
				bool 	dirty_flag,
				bool 	valid,
				file* 	file_stat,
				Common::allocator& _allocator)
				throw(std::bad_alloc);
			~block();
			block(const block&);

			size_t allocate_memory()throw(std::bad_alloc);
			bool need_allocation()const;
			size_t free_memory();
			void set_to_existing();
			bool is_receiving_data();
			void set_to_receive_data();
			void finish_transfer();
			int dataRdlock();
			int dataWrlock();
			int dataunlock();

			int metalock();
			int metaunlock();

			size_t 			data_size;	/*the size of data in the block*/
			size_t			block_size;	/*the size of the block*/
			void* 			data;		/*the pointer to the data*/
			off64_t 		start_point;	/*the start point of the data in the file*/
			bool 			dirty_flag;	/*if it is dirty*/
			bool 			valid;		/*if the memory is allocated*/
			bool			swapout_flag;	/*if the block is swapped out*/
			volatile bool		writing_back;	/*if the block is under writing back. no swap if this flag is not 0*/
			volatile bool		receiving_data;	/*if the block is under receiving. no swap if this flag is set*/
			volatile int		registered;	/*if the block is registered. register if this flag is not 0*/
			int 			exist_flag;	/*if the file exist on remote disk*/
			file*			file_stat;	/*the pointer to the file status*/
			//Common::remote_task* 	write_back_task;
			pthread_rwlock_t	data_rwlock;		/*lock for data*/
			pthread_mutex_t		meta_lock;		/*lock for metadata*/
			//remote handler will delete this struct if TO_BE_DELETED is setted
			//this appends when user unlink file while remote handler is writing back
			int			postponed_operation;
			Common::
			access_page<block>*	writeback_page;	/*the pointer to element in the swapping queue*/
			Common::allocator&	memory_allocator;/*memory allocator*/
			Common::memory_elem*	_elem;		/*the element in for the memory allocation*/
		};

		typedef std::map<off64_t, block*> block_info_t; //map: start_point : block*
		typedef std::map<ssize_t, Common::comm_handle_t> handle_ptr_pool_t; //map node id: handle

		struct file
		{
			file(const char *path,
			     int exist_flag, 
			     ssize_t file_no);
			~file();
			int lock();
			int unlock();
			int add_dirty_pages(int count);
			int remove_dirty_pages(int count);

			std::string 		file_path;
			int 			exist_flag;
			bool			close_flag;
			bool 			dirty_flag;
			int			postponed_operation;
			int			dirty_pages_count;
			int 			main_flag; //main replica indicator
			ssize_t 		file_no;
			block_info_t  		blocks;
			handle_ptr_pool_t 	IOnode_pool;
			int			read_remote_fd;
			int			write_remote_fd;
		private:
			pthread_mutex_t 	_lock;
		};

		inline int block::
			dataRdlock()
		{
			struct timeval st, et;
			if(0 != pthread_rwlock_tryrdlock(&data_rwlock))
			{
				gettimeofday(&st, nullptr);
				while(0 != pthread_rwlock_tryrdlock(&data_rwlock))
				{
					this->metaunlock();
					this->metalock();
				}
				gettimeofday(&et, nullptr);
				_LOG("read lock %p for time %f\n", this, TIME(st, et));
			}
			return SUCCESS;
		}

		inline int block::
			dataWrlock()
		{
			struct timeval st, et;
			if(0 != pthread_rwlock_trywrlock(&data_rwlock))
			{
				gettimeofday(&st, nullptr);
				//bad hack 
				//find a good way to remove this metalock -> datalock deadlock
				while(0 != pthread_rwlock_trywrlock(&data_rwlock))
				{
					this->metaunlock();
					this->metalock();
				}
				gettimeofday(&et, nullptr);
				_LOG("write lock %p for time %f\n", this, TIME(st, et));
			}
			return SUCCESS;
		}

		inline int block::
			dataunlock()
		{
			return pthread_rwlock_unlock(&data_rwlock);
		}

		inline int block::
			metalock()
		{
			struct timeval st, et;
			if(0 != pthread_mutex_trylock(&meta_lock))
			{
				gettimeofday(&st, nullptr);
				while(0 != pthread_mutex_trylock(&meta_lock));
				gettimeofday(&et, nullptr);
				_LOG("meta lock %p for time %f\n", this, TIME(st, et));
			}
			return SUCCESS;
		}

		inline int block::
			metaunlock()
		{
			return pthread_mutex_unlock(&meta_lock);
		}

		inline bool block::
			need_allocation()const
		{
			return INVALID == this->valid ||
				nullptr == data;
		}

		inline	void block::
			set_to_existing()
		{
			exist_flag=EXISTING;
			file_stat->exist_flag=EXISTING;
		}

		inline int file::
			lock()
		{
			return pthread_mutex_lock(&_lock);
		}

		inline int file::
			add_dirty_pages(int count)
		{
			lock();
			dirty_pages_count+=count;
			unlock();
			return count;
		}

		inline int file::
			remove_dirty_pages(int count)
		{
			lock();
			dirty_pages_count-=count;
			unlock();
			return count;
		}

		inline int file::
			unlock()
		{
			return pthread_mutex_unlock(&_lock);
		}

		inline bool block::
			is_receiving_data()
		{
			return SET == this->receiving_data;
		}

		inline void block::
			set_to_receive_data()
		{
			this->receiving_data=SET;
		}

		inline void block::
			finish_transfer()
		{
			this->metalock();
			this->receiving_data=UNSET;
			this->dataunlock();

			this->metaunlock();
		}
	}
}
#endif
