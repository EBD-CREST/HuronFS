/*
 * Copyright (c) 2017, Lawrence Livermore National Security, LLC. Produced at
 * the Lawrence Livermore National Laboratory. 
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp, LLNL-CODE-722817.
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

#include "Comm_basic.h"

using namespace std;
using namespace CBB::Common;

int Comm_basic::
	create_socket(const struct sockaddr_in& addr)
throw(std::runtime_error)
{
	int 		sockfd = 0;
	int 		on     = 1; 
	struct timeval  timeout;      
	
	set_timer(&timeout);

	if( 0 > (sockfd = socket(PF_INET,  SOCK_STREAM,  0)))
	{
		perror("Create Socket Failed");  
		throw std::runtime_error("Create Socket Failed");   
	}

	if (setsockopt(sockfd,  SOL_SOCKET,
				SO_REUSEADDR,  &on,  sizeof(on) ))
	{
		perror("setsockopt failed");
		throw std::runtime_error("sockopt Failed");   
	}

	if (setsockopt(sockfd, SOL_SOCKET,
				SO_RCVTIMEO, (char *)&timeout,
				sizeof(timeout)) < 0)
	{
		perror("setsockopt failed");
		throw std::runtime_error("sockopt Failed");   
	}

	if(0 != bind(sockfd, 
				reinterpret_cast<const struct sockaddr*>(&addr),
				sizeof(addr)))
	{
		perror("Server Bind Port Failed"); 
		throw std::runtime_error("Server Bind Port Failed");  
	}

	return sockfd;
}

