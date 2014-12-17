#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <sys/types.h>

#include "include/IOnode.h"
#include "include/IO_const.h"

int main(int argc, char**argv)
{
/*	if( 2 > argc)
	{
		fprintf(stderr, "too few parameters\n"); 
		exit(0); 
	}*/

	const char *_ip=NULL;
	if(NULL == (_ip=getenv("BB_MASTER_IP")))
	{
		fprintf(stderr, "please set master ip\n");
		return EXIT_FAILURE;
	}
	std::string master_ip(_ip); 
	try
	{
		IOnode node(master_ip, MASTER_PORT); 
		node.start_server();
	}
	catch(std::runtime_error& e)
	{
		fprintf(stderr, e.what());
		exit(1); 
	}
}
