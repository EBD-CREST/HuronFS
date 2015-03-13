#ifndef _CBB_INTERNAL_H_

#define _CBB_INTERNAL_H_

#ifdef DEBUG
	#define _DEBUG(fmt, args... ) printf("[%s] "fmt, __func__, ##args)
	#define _LOG(fmt, args... ) printf("[%s]"fmt, __func__, ##args)
	#define _ERROR(fmt, args... ) printf("[%s]"fmt, __func__, ##args)
#else
	#define _DEBUG(fmt, args... )
	#define _LOG(fmt, args... ) 
	//#define _LOG(fmt, args... ) printf("[%s]"fmt, __func__, ##args)
	#define _ERROR(fmt, args... ) printf("[%s]"fmt, __func__, ##args)
#endif


#ifdef CBB_PRELOAD
	#ifndef __USE_GNU
		#define __USE_GNU
	#endif
	
	#include <dlfcn.h>
	#include <stdlib.h>

	#define CBB_WRAP(name) name

	#define CBB_REAL(name) __real_P_ ## name

	#define	CBB_FUNC_P(ret,name,args)                                                                        \
		typedef ret (*__real_ ## name) args;                                                            \
		__real_ ## name __real_P_ ## name=NULL;

	#define MAP_BACK(func)                                                                                  \
		if (!(__real_P_ ## func))                                                                       \
		{                                                                                               \
			__real_P_ ## func = reinterpret_cast<__real_ ## func>(dlsym(RTLD_NEXT, #func));         \
			if(!(__real_P_## func))                                                                 \
			{                                                                                       \
				fprintf(stderr, "CBB failed to map symbol: %s\n", #func);                        \
				exit(1);                                                                        \
			}                                                                                       \
		}


#else

	#define CBB_WRAP(name) __warp_ ## name

	#define CBB_REAL(name) __real_ ## name

	#define MAP_BACK(func) 

	#define	CBB_FUNC_P(ret,name,args)                                                                        \
		extern ret __real_ ##name args;
#endif

#define CHECK_INIT()                                                      \
	do                                                                \
	{                                                                 \
		if(!_initial)                                             \
		{                                                         \
			fprintf(stderr, "initialization unfinished\n");   \
			errno = EAGAIN;                                   \
			return -1;                                        \
		}                                                         \
	}while(0)

#define MIN(a,b) ((a)>(b)?(b):(a))

#include <arpa/inet.h>
#include <string.h>

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
#endif
