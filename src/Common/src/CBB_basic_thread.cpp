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


#include "CBB_basic_thread.h"

#include <sched.h>

using namespace CBB;
using namespace CBB::Common;


CBB_basic_thread::
CBB_basic_thread(int thread_number):
	thread_number(thread_number),
	keepAlive_flag(NOT_KEEP_ALIVE),
	thread_id()
{}

CBB_basic_thread::
~CBB_basic_thread()
{}

int CBB_basic_thread::
create_thread(void* (*thread_fun)(void*), void* argument)
{
	int ret=0;

	keepAlive_flag = KEEP_ALIVE;
	if(0 == (ret=pthread_create(&thread_id, nullptr, thread_fun, argument)))
	{
		return SUCCESS;
	}
	else
	{
		perror("pthread_create");
		return FAILURE;
	}

	setaffinity();

}

int CBB_basic_thread::
setaffinity()
{
	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	CPU_SET(thread_number, &cpu_set);
	
	return pthread_setaffinity_np(thread_id, sizeof(cpu_set), &cpu_set);

}

int CBB_basic_thread::
init_thread()
{
	return SUCCESS;
}

int CBB_basic_thread::
end_thread()
{
	keepAlive_flag = NOT_KEEP_ALIVE;
	void* ret=nullptr;

	return pthread_join(thread_id, &ret);
}
