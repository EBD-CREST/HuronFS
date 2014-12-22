#ifndef _BB_INTERNAL_H_

#define _BB_INTERNAL_H_

#ifdef DEBUG
	#define debug(fmt, args... ) printf("%s:"fmt, __func__, ##args)
#else
	#define debug(fmt, args... )
#endif


#ifdef BB_PRELOAD
	#ifndef __USE_GNU
		#define __USE_GNU
	#endif
	
	#include <dlfcn.h>
	#include <stdlib.h>

	#define BB_WRAP(name) name

	#define BB_REAL(name) __real_P_ ## name

	#define	BB_FUNC_P(ret,name,args)                                                                        \
		typedef ret (*__real_ ## name) args;                                                            \
		__real_ ## name __real_P_ ## name=NULL;

	#define MAP_BACK(func)                                                                                  \
		if (!(__real_P_ ## func))                                                                       \
		{                                                                                               \
			__real_P_ ## func = reinterpret_cast<__real_ ## func>(dlsym(RTLD_NEXT, #func));         \
			if(!(__real_P_## func))                                                                 \
			{                                                                                       \
				fprintf(stderr, "BB failed to map symbol: %s\n", #func);                        \
				exit(1);                                                                        \
			}                                                                                       \
		}


#else

	#define BB_WRAP(name) __warp_ ## name

	#define BB_REAL(name) __real_ ## name

	#define MAP_BACK(func) 

	#define	BB_FUNC_P(ret,name,args)                                                                        \
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

#include <arpa/inet.h>
#include <string.h>

inline void set_server_addr(const std::string &ip, struct sockaddr_in &addr)
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
