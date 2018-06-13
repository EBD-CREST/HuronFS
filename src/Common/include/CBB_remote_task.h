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

#ifndef CBB_REMOTE_TASK_H_
#define CBB_REMOTE_TASK_H_

#include "CBB_task_parallel.h"
#include "CBB_basic_thread.h"

namespace CBB
{
	namespace Common
	{

		class remote_task:public basic_task
		{
			public:
				remote_task();
				remote_task(int id, remote_task* next);
				virtual ~remote_task()=default;
				void set_task_id(int id);
				int get_task_id()const;
				void set_task_file_name(const std::string& task_file_name);
				const std::string& get_task_file_name()const;
				void set_extended_file_name(const std::string& task_file_name);
				const std::string& get_extended_file_name()const;
				void set_mode(mode_t mode);
				mode_t get_mode();
			private:
				int 		task_id;
				std::string	task_file_name;
				std::string	extended_file_name;
				mode_t		mode;
		};

		class CBB_remote_task:
			public CBB_basic_thread
		{
			public:
				CBB_remote_task();
				virtual ~CBB_remote_task();
				virtual int remote_task_handler(remote_task* new_task)=0;
				int start_listening();
				void stop_listening();
				remote_task* add_remote_task(int task_code, const std::string& task_file_name, mode_t mode);
				remote_task* add_remote_task(int task_code, 
						const std::string& task_file_name, 
						const std::string& extended_file_name, mode_t mode);
				int remote_task_enqueue(remote_task* new_task);
				void remote_task_dequeue(remote_task* new_task);
				static void* thread_fun(void* args);
			private:
				bool 		thread_started;
				task_parallel_queue<remote_task> remote_task_queue;
				pthread_mutex_t locker;
		};

		inline void remote_task::set_task_id(int task_id)
		{
			this->task_id=task_id;
		}

		inline int remote_task::get_task_id()const
		{
			return this->task_id;
		}

		inline void remote_task::set_task_file_name(const std::string& task_file_name)
		{
			this->task_file_name=task_file_name;
		}

		inline const std::string& remote_task::get_task_file_name()const
		{
			return this->task_file_name;
		}

		inline void remote_task::set_extended_file_name(const std::string& file_name)
		{
			this->extended_file_name=file_name;
		}

		inline const std::string& remote_task::get_extended_file_name()const
		{
			return this->extended_file_name;
		}

		inline void remote_task::set_mode(mode_t mode)
		{
			this->mode=mode;
		}

		inline mode_t remote_task::get_mode()
		{
			return this->mode;
		}

		inline int CBB_remote_task::remote_task_enqueue(remote_task* new_task)
		{
			return remote_task_queue.task_enqueue_signal_notification();
		}

		inline void CBB_remote_task::remote_task_dequeue(remote_task* new_task)
		{
			remote_task_queue.task_dequeue();
		}
	}
}

#endif
