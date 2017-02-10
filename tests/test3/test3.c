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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <sys/time.h>
#include <linux/limits.h>

//access to a remote file using MPI
//test function:
//open, read, write, flush, close

const int ROOT=0;

#define TIME(st, et) (((et).tv_sec-(st).tv_sec)*1000000+(et).tv_usec-(st).tv_usec)

int main(int argc, char ** argv)
{
	int fd;
	int rank, number, i=0;
	char *buffer=NULL;
	size_t total_size=0;
	ssize_t ret;
	struct stat file_stat;
	char   test;
	struct timeval st, et;
	char file_path[PATH_MAX];
	const char *mount_point=getenv("CBB_CLIENT_MOUNT_POINT");

	if(NULL == mount_point)
	{
		fprintf(stderr, "please set CBB_CLIENT_MOUNT_POINT");
		return EXIT_FAILURE;
	}
	sprintf(file_path, "%s%s", mount_point, "/test1");

	fd=open(file_path, O_WRONLY);
	fstat(fd, &file_stat);
	if(NULL == (buffer=malloc(file_stat.st_size)))
	{
		perror("malloc");
		return EXIT_FAILURE;
	}

	gettimeofday(&st, NULL);

	if(-1 == (ret=write(fd, buffer, file_stat.st_size)))
	{
		perror("read");
		return EXIT_FAILURE;
	}
	if(-1 == (ret=lseek(fd, 0, SEEK_SET)))
	{
		perror("lseek");
		return EXIT_FAILURE;
	}
	printf("seek to begin\n");
	printf("read again\n");
	scanf("%c", &test);
	if(-1 == (ret=write(fd, buffer, file_stat.st_size)))
	{
		perror("read");
		return EXIT_FAILURE;
	}
	total_size+=ret;

	gettimeofday(&et, NULL);

	printf("total size %lu\n", total_size);
	printf("throughput %fMB/s\n", (double)total_size/TIME(st, et));

	return EXIT_SUCCESS;
}
