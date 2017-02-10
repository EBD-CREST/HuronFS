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

#ifndef CBB_FAULT_TOLERANCE_H_
#define CBB_FAULT_TOLERANCE_H_

#include "Comm_basic.h"
#include "CBB_IO_task.h"

namespace CBB
{
	namespace Common
	{
		class CBB_fault_tolerance
		{
			public:
				CBB_fault_tolerance()=default;
				~CBB_fault_tolerance()=default;
				virtual int node_failure_handler(extended_IO_task* task)=0;
				virtual int node_failure_handler(comm_handle_t handle)=0;
				virtual int connection_failure_handler(extended_IO_task* task)=0;
		};
	}
}

#endif
