#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <sys/types.h>

#include "IOnode.h"
#include "CBB_const.h"

using namespace CBB::Common;
using namespace CBB::IOnode;
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
	int run_daemon=true;
	while(-1 != (oc=getopt(argc, argv, optstring)))
	{
		switch(oc)
		{
			case 'd':
				run_daemon=false;break;
				break;
			case 'h':
				usage();exit(EXIT_SUCCESS);break;
			case '?':
				fprintf(stderr, "unknown option %c\n", (char)optopt);
				usage();exit(EXIT_SUCCESS);
		}

	}
	return run_daemon;
}

int main(int argc, char**argv)
{
	const char *_ip=NULL;
	if(NULL == (_ip=getenv("HUFS_MASTER_IP")))
	{
		fprintf(stderr, "please set master ip\n");
		return EXIT_FAILURE;
	}
	std::string master_ip(_ip); 
	try
	{
		IOnode node(master_ip, MASTER_PORT); 
		if(parse_opt(argc, argv))
		{
			if(-1 == daemon(0, 0))
			{
				perror("daemon error");
				return EXIT_FAILURE;
			}
		}
		node.IOnode::start_server();
	}
	catch(std::runtime_error&e)
	{
		fprintf(stderr, e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
