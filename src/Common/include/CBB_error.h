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

#ifndef CBB_ERROR_H_
#define CBB_ERROR_H_

#include <exception>
#include <string>

namespace CBB
{
	typedef int CBB_error; 

	class CBB_socket_exception: public std::runtime_error
	{
		public:
		explicit CBB_socket_exception(const std::string& message):
			std::runtime_error(message){}
		explicit CBB_socket_exception(const char* message):
			std::runtime_error(message){}
		virtual const char* what()
		{
			return std::runtime_error::what();
		}
	};

	class CBB_bad_alloc: public std::bad_alloc
	{
		public:
		virtual const char* what()
		{
			return "out of memory";
		}
	};

	class CBB_parameter_error: public std::runtime_error
	{
		public:
		explicit CBB_parameter_error(const std::string& message):
			std::runtime_error(message){}
		explicit CBB_parameter_error(const char* message):
			std::runtime_error(message){}
		virtual const char* what()
		{
			return std::runtime_error::what();
		}
	};

	class CBB_configure_error: public std::runtime_error
	{
		public:
		explicit CBB_configure_error(const std::string& message):
			std::runtime_error(message){}
		explicit CBB_configure_error(const char* message):
			std::runtime_error(message){}
		virtual const char* what()
		{
			return std::runtime_error::what();
		}
	};
}
#endif
