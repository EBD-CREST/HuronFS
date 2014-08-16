#include <stdlib.h>
#include <stdio.h>

#include "include/Master.h"

int main(int argc, char **argv)
{
	try
	{
		Master master; 
		master.start_server(); 
		master.stop_server(); 
	}
	catch(std::runtime_error &e)
	{
		fprintf(stderr, "%s\n", e.what()); 
		exit(1); 
	}
	return 0; 
}
