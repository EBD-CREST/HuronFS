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
