#ifndef _BB_INTERNAL_H_

#define _BB_INTERNAL_H_

#ifdef DEBUG
	#define debug(fmt, args... ) printf("%s:"fmt, __func__, ##args)
#else
	#define debug(fmt, args... )
#endif


#ifdef BB_PRELOAD
	#define __USE_GNU
	#include <dlfcn.h>
	#include <stdlib.h>

	#define BB_WRAP(name) name

	#define BB_REAL(name) __real_ ## name

	#define MAP_BACK(func) \
		if (!(__real_ ## func)) \
	{\
		__real_ ## func = dlsym(RTLD_NEXT, #func); \
		if(!(__real_ ## func)) {\
			fprintf(stderr, "BB failed to map symbol: %s\n", #func); \
			exit(1); \
		} \
	}

	#define	BB_FUNC_P(ret,name,args) ret (*__real_ ## name) args=NULL;

#else

	#define BB_WRAP(name) __warp_ ## name

	#define BB_REAL(name) __real_ ## name

	#define MAP_BACK(func) 

	#define	BB_FUNC_P(ret,name,args) \
		extern ret __real_ ##name args;
#endif

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
