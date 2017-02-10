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

#ifndef CBB_BASIC_H_
#define CBB_BASIC_H_

#include <map>
#include <set>
#include <string>
#include <sys/stat.h>

#include "CBB_error.h"
#include "CBB_const.h"
#include "Comm_api.h"

namespace CBB
{
	namespace Client
	{
		class block_info;
		class file_meta;
		class opened_file_info;
		class CBB_client;

		typedef std::map<ssize_t, Common::comm_handle> IOnode_fd_map_t;
		typedef std::set<int> _opened_fd_t;
		typedef std::map<std::string, file_meta*> _path_file_meta_map_t; //map path, fd
		typedef std::vector<ssize_t> IOnode_list_t;
		//typedef std::vector<block_info> _block_list_t;
		typedef std::map<off64_t, size_t> _block_list_t;

		_block_list_t::const_iterator
			find_block_by_start_point(
					_block_list_t 	blocks,
					off64_t		start_point);

		class SCBB
		{
			public:
				friend class CBB_client;
				friend class file_meta;

				SCBB()=default;
				SCBB(int id, Common::comm_handle_t master_handle);
				~SCBB()=default;
				Common::comm_handle_t get_IOnode_fd(ssize_t IOnode_id);				
				void set_id(int id);				
				void set_master_handle(Common::comm_handle_t handle);
				Common::comm_handle_t get_master_handle();
				IOnode_fd_map_t& get_IOnode_list();
				Common::comm_handle_t insert_IOnode(ssize_t IOnode_id,
							Common::comm_handle_t IOnode_handle);
				Common::comm_handle& get_IOnode_handle(ssize_t IOnode_id);
			private:
				IOnode_fd_map_t       IOnode_list; //map IOnode id: fd
				int 		      id;
				Common::comm_handle   master_handle;
		};

		class  block_info
		{
			public:
				friend class CBB_client;
				friend _block_list_t::const_iterator
					find_block_by_start_point(
							_block_list_t 	blocks,
							off64_t		start_point);
				block_info(off64_t start_point, size_t size);
				friend bool operator == (const block_info& src, const block_info& des);
			private:
				//ssize_t node_id;
				off64_t start_point;
				size_t 	size;
		};

		class file_meta
		{
			public:
				friend class CBB_client;
				friend class opened_file_info;
				file_meta(ssize_t remote_file_no,
						size_t block_size,
						const struct stat* file_stat,
						SCBB* corresponding_SCBB);
				Common::comm_handle_t get_master_handle();
				int get_master_number();
			private:
				ssize_t 	remote_file_no;
				int 		open_count;
				size_t 		block_size;
				struct stat 	file_stat;
				_opened_fd_t 	opened_fd;
				SCBB		*corresponding_SCBB;
				//int 		master_handle;
				_path_file_meta_map_t::iterator it;

		};

		class opened_file_info
		{
			public:
				friend class CBB_client;
				opened_file_info(int fd, int flag, file_meta* file_meta_p);
				opened_file_info();
				opened_file_info(const opened_file_info& src);
				~opened_file_info();
				struct stat& get_file_metadata();
				const struct stat& get_file_metadata()const;
				ssize_t get_remote_file_no()const;
			private:
				const opened_file_info& operator=(const opened_file_info&)=delete;
				//current offset in file
				off64_t 	current_point;
				int 		fd;
				int 		flag;
				file_meta* 	file_meta_p;
				IOnode_list_t	IOnode_list_cache;
				_block_list_t	block_list;
		};


		inline struct stat& opened_file_info::
			get_file_metadata()
		{
			return this->file_meta_p->file_stat;
		}
		inline const struct stat& opened_file_info::
			get_file_metadata()const
		{
			return this->file_meta_p->file_stat;
		}

		inline _block_list_t::const_iterator
			find_block_by_start_point(_block_list_t blocks,
						  off64_t	start_point)
		{
			_block_list_t::const_iterator it=
				blocks.find(start_point);
			/*for(;it != std::end(blocks) &&
				it->start_point != start_point;
				it ++);*/
			return it;
		}

		inline void SCBB::
			set_id(int id)	
		{
			this->id=id;
		}

		inline void SCBB::
			set_master_handle(Common::comm_handle_t handle)	
		{
			this->master_handle=*handle;
		}

		inline Common::comm_handle_t file_meta::
			get_master_handle()
		{
			return &this->corresponding_SCBB->master_handle;
		}

		inline int file_meta::
			get_master_number()
		{
			return this->corresponding_SCBB->id;
		}
		inline Common::comm_handle_t SCBB::
			insert_IOnode(ssize_t IOnode_id,
				      Common::comm_handle_t IOnode_handle)
		{
			Common::comm_handle_t ret=
				&(this->IOnode_list.insert(std::make_pair(IOnode_id, *IOnode_handle)).first->second);
			return ret;
		}

		inline IOnode_fd_map_t& SCBB::
			get_IOnode_list()
		{
			return this->IOnode_list;
		}

		inline Common::comm_handle& SCBB::
			get_IOnode_handle(ssize_t IOnode_id)
		{
			return this->IOnode_list[IOnode_id];
		}

		inline Common::comm_handle_t SCBB::
			get_master_handle()
		{
			return &this->master_handle;
		}
		inline ssize_t opened_file_info::
			get_remote_file_no()const
		{
			return this->file_meta_p->remote_file_no;
		}

		inline bool operator == (const block_info& src, const block_info& des)
		{
			return src.start_point == des.start_point;
		}
	}
}
#endif
