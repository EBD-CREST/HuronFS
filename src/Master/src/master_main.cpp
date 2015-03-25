#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "Master.h"
const char optstring[]="dh";

void usage()
{
	printf("usage:\n");
	printf("-d: run as daemon\n");
	printf("-h: print this information\n");
	return;
}

int parse_opt(int argc, char ** argv)
{
	int oc;
	int run_daemon=false;
	while(-1 != (oc=getopt(argc, argv, optstring)))
	{
		switch(oc)
		{
			case 'd':
				run_daemon=true;break;
				break;
			case 'h':
				usage();exit(0);exit(EXIT_SUCCESS);break;
			case '?':
				fprintf(stderr, "unknown option %c\n", (char)optopt);
				usage();exit(EXIT_SUCCESS);
		}

	}
	return run_daemon;
}


int main(int argc, char **argv)
{
	if(parse_opt(argc, argv))
	{
		if(-1 == daemon(0, 0))
		{
			perror("daemon error");
			return EXIT_FAILURE;
		}
	}
	try
	{
		Master master; 
		master.start_server(); 
		master.stop_server(); 
	}
	catch(std::runtime_error &e)
	{
		fprintf(stderr, "%s\n", e.what()); 
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS; 
}
