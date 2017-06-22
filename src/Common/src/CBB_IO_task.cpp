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

#include "CBB_IO_task.h"

using namespace CBB::Common;
using namespace std;

basic_IO_task::basic_IO_task():
	//base class
	basic_task(),
	connection_task(),
	//fields
	handle(),
	message_buffer(),
	error(),
	receiver_id(reinterpret_cast<int*>(message_buffer+RECEIVER_ID_OFF)),
	sender_id(reinterpret_cast<int*>(message_buffer+SENDER_ID_OFF)),
	message_size(reinterpret_cast<size_t*>(message_buffer+MESSAGE_SIZE_OFF)),
	basic_message(message_buffer+MESSAGE_META_OFF),
	current_point(0)
{
	set_message_size(0);
	*sender_id=get_id();
}

basic_IO_task::
basic_IO_task(int id, basic_IO_task* next):
	//base class
	basic_task(id, next),
	connection_task(),
	//fields
	handle(),
	message_buffer(),
	error(),
	receiver_id(reinterpret_cast<int*>(message_buffer+RECEIVER_ID_OFF)),
	sender_id(reinterpret_cast<int*>(message_buffer+SENDER_ID_OFF)),
	message_size(reinterpret_cast<size_t*>(message_buffer+MESSAGE_SIZE_OFF)),
	basic_message(message_buffer+MESSAGE_META_OFF),
	current_point(0)
{
	set_message_size(0);
	*sender_id=get_id();
}

send_buffer_element::
send_buffer_element(char* buffer, size_t size):
	buffer(buffer),
	size(size)
{}

extended_IO_task::
extended_IO_task():
	//base class
	basic_IO_task(),
	//fields
	mode(RMA_READ),
	extended_size(reinterpret_cast<size_t*>(get_message()+EXTENDED_SIZE_OFF)),
	send_buffer(),
	receive_buffer(nullptr),
	current_receive_buffer_ptr(nullptr),
	extended_buffer()
{}

extended_IO_task::
extended_IO_task(int id, extended_IO_task* next):
	//base class
	basic_IO_task(id, next),
	//fields
	mode(RMA_READ),
	extended_size(reinterpret_cast<size_t*>(get_message()+EXTENDED_SIZE_OFF)),
	send_buffer(),
	receive_buffer(nullptr),
	current_receive_buffer_ptr(nullptr),
	extended_buffer()
{}

extended_IO_task::~extended_IO_task()
{
	if(nullptr != receive_buffer)
	{
		delete[] receive_buffer;
	}
}

