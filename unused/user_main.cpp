#include <stdlib.h>
#include <stdio.h>

#include "include/Query_Client.h"

int main(int argc, char**argv)
{
	if(3 > argc)
	{
		fprintf(stderr, "too few parameters\n"); 
		exit(0); 
	}
	Query_Client client(0); 
	try
	{
		client.set_server_addr(argv[1], atoi(argv[2])); 
	}
	catch(std::runtime_error &e)
	{
		fprintf(stderr, "%s\n", e.what()); 
		exit(0); 
	}
	while(1)
	{
		client.parse_query(); 
	}
	return 0;
}

