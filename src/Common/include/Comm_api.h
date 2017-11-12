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

#ifndef COMM_API_H_
#define COMM_API_H_

#ifdef TCP

#include "comm_type/tcp.h"

namespace CBB
{
	namespace Common
	{
		typedef CBB_tcp CBB_communication;
		typedef TCP_handle CBB_handle;
	}
}
#else

#include "comm_type/cci.h"

namespace CBB
{
	namespace Common
	{
		typedef CBB_cci CBB_communication;
		typedef CCI_handle CBB_handle;
	}
}

#endif

namespace CBB
{
	namespace Common
	{

		typedef CBB_handle comm_handle;
		typedef CBB_handle* comm_handle_t;
		typedef const CBB_handle* const_comm_handle_t;
		typedef CBB_handle& ref_comm_handle_t;
		typedef const CBB_handle& const_ref_comm_handle_t;
	}
}

#endif
