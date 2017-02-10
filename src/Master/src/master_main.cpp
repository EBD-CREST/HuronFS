/*
 * Copyright (c) 2017, Tokyo Institute of Technology
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
 * All rights reserved. 
 * 
 * This file is part of HuronFS.
 * 
 * Please also read the file "LICENSE" included in this package for 
 * Our Notice and GNU Lesser General Public License.
 * 
 * This program is free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License (as published by the Free Software 
 * Foundation) version 2.1 dated February 1999. 
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "Master.h"
const char optstring[]="dh";
using namespace CBB::Master;

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
	try
	{
		Master master;
		if(parse_opt(argc, argv))
		{
			if(-1 == daemon(0, 0))
			{
				perror("daemon error");
				return EXIT_FAILURE;
			}
		}
		master.start_server(); 
		master.stop_server(); 
	}
	catch(std::exception &e)
	{
		fprintf(stderr, 
			"ERROR !! Master set up failed\n%s\n",
			e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS; 
}
