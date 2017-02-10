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

#ifndef CBB_HEART_BEAT_H_
#define CBB_HEART_BEAT_H_

#include "CBB_task_parallel.h"
#include "CBB_communication_thread.h"

namespace CBB
{
	namespace Common
	{
		class CBB_heart_beat
		{
			public:
				//socket, IOnode_id
				typedef std::map<comm_handle_t, ssize_t> handle_map_t;
				CBB_heart_beat();
				CBB_heart_beat(communication_queue_t* input_queue,
						communication_queue_t* output_queue);	
				virtual ~CBB_heart_beat() = default;	

				void set_queues(communication_queue_t* input_queue,
						communication_queue_t* output_queue);
				int heart_beat_check();

				void heart_beat_func();

				int send_heart_beat_check(comm_handle_t handle);
				virtual int get_IOnode_handle_map(handle_map_t& map)=0;
			private:
				CBB_heart_beat(const CBB_heart_beat&) = delete;	
				CBB_heart_beat& operator=(const CBB_heart_beat&) = delete;

			private:
				int keepAlive;
				communication_queue_t* input_queue;
				communication_queue_t* output_queue;
		};
	}
}

#endif
