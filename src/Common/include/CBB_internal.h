#ifndef _CBB_INTERNAL_H_

#define _CBB_INTERNAL_H_

#ifdef DEBUG
	#define _DEBUG(fmt, args... ) fprintf(stderr, "[%s]" fmt, __func__, ##args)
	#define _ERROR(fmt, args... ) fprintf(stderr, "[%s]" fmt, __func__, ##args)
#else
	#define _DEBUG(fmt, args... )
	#define _ERROR(fmt, args... ) fprintf(stderr, "[%s]" fmt, __func__, ##args)
#endif

#ifdef LOG
	#define _LOG(fmt, args... ) fprintf(stderr, "[%s]" fmt, __func__, ##args)
#else
	#define _LOG(fmt, args... ) 
#endif


#ifdef CBB_PRELOAD
	#ifndef __USE_GNU
		#define __USE_GNU
	#endif
	
	#include <dlfcn.h>
	#include <stdlib.h>

	#define CBB_WRAP(name) name

	#define CBB_REAL(name) __real_P_ ## name

	#define	CBB_FUNC_P(ret,name,args)                                                                       \
		typedef ret (*__real_ ## name) args;                                                            \
		__real_ ## name __real_P_ ## name=NULL;

	#define MAP_BACK(func)                                                                                  \
		if (!(__real_P_ ## func))                                                                       \
		{                                                                                               \
			__real_P_ ## func = reinterpret_cast<__real_ ## func>(dlsym(RTLD_NEXT, #func));         \
			if(!(__real_P_## func))                                                                 \
			{                                                                                       \
				fprintf(stderr, "CBB failed to map symbol: %s\n", #func);                       \
				exit(EXIT_FAILURE);                                                             \
			}                                                                                       \
		}


#else

	#define CBB_WRAP(name) __warp_ ## name

	#define CBB_REAL(name) __real_ ## name

	#define MAP_BACK(func) 

	#define	CBB_FUNC_P(ret,name,args)                                                                        \
		extern ret __real_ ##name args;
#endif


#define MIN(a,b) ((a)>(b)?(b):(a))
#define MAX(a,b) ((a)<(b)?(b):(a))

#include <arpa/inet.h>
#include <string>
#include <stdio.h>
#include <functional>
#include <string.h>

#include "CBB_const.h"

inline int file_path_hash(const std::string& file_path, int number)
{
	static std::hash<std::string> string_hash;
	return string_hash(file_path)%number;
}

inline void set_server_addr(const std::string& ip, struct sockaddr_in& addr)
{
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(IONODE_PORT);
	if(0 == inet_aton(ip.c_str(), &addr.sin_addr))
	{
		perror("IOnode Address Error");
	}
	return;
}

inline size_t get_block_size(size_t size)
{
	return BLOCK_SIZE;
}
inline off64_t get_block_start_point(off64_t start_point,
		size_t& size)
{
	off64_t block_start_point=(start_point/BLOCK_SIZE)*BLOCK_SIZE;
	size=start_point-block_start_point+size;
	return block_start_point;
}

inline off64_t get_block_start_point(off64_t start_point)
{
	off64_t block_start_point=(start_point/BLOCK_SIZE)*BLOCK_SIZE;
	return block_start_point;
}

inline char* parse_master_config_ip(char* start_ptr,
				    char* des_ptr)
{
	char* next_master_ip=strchr(start_ptr, ',');
	char* ret=nullptr;

	if(0 == *start_ptr)
	{
		return ret;
	}
	if(nullptr == next_master_ip)
	{
		for(next_master_ip=start_ptr;
				'\0' != *next_master_ip; ++next_master_ip);
		strncpy(des_ptr, start_ptr, next_master_ip-start_ptr);
		des_ptr[next_master_ip-start_ptr]=0;
		ret=next_master_ip;
	}
	else
	{
		strncpy(des_ptr, start_ptr, next_master_ip-start_ptr);
		des_ptr[next_master_ip-start_ptr]=0;
		ret=next_master_ip+1;
	}
	return ret;
}

inline const char* parse_master_config_ip(const char* start_ptr,
					  char*	      des_ptr)
{
	return parse_master_config_ip(const_cast<char*>(start_ptr), des_ptr);	
}

#endif
