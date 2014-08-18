#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <sys/types.h>

#include "include/IOnode.h"
#include "include/IO_const.h"

int main(int argc, char**argv)
{
	if( 3 > argc)
	{
		fprintf(stderr, "too few parameters\n"); 
		exit(0); 
	}

	std::string master_ip(argv[1]), my_ip(argv[2]);
	try
	{
		IOnode node(master_ip, my_ip, MASTER_PORT); 
		node.start_server();
	}
	catch(std::runtime_error& e)
	{
		fprintf(stderr, "IOnode regist Failed\n");
		exit(1); 
	}
}