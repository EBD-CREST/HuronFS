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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//test buffer IO API
//test function:
//fopen, fread, fwrite, fflush, fclose

void fill_buf(char *buf, size_t size)
{
	static char value='0';
	size_t i=0;
	for(i=0; i<size; i++)
	{
		buf[i]=value;
	}
	value++;
}

int main(int argc, char ** argv)
{
	int fd=-1;
	//int buffer;
	char *buffer=NULL;
	size_t size=104857600;
	int i=0, loop=0;
	if(argc < 3)
	{
		fprintf(stderr, "usage test4 [file_path] [size] [loop]\n");
		return EXIT_FAILURE;
	}
	if(-1 == (fd=creat(argv[1], 0600)))
	{
		perror("open");
		return EXIT_FAILURE;
	}
	size=atoi(argv[2]);
	loop=atoi(argv[3]);
	printf("file size = %ld\n", size);
	printf("loop = %d\n", loop);
	if(NULL == (buffer=malloc(size)))
	{
		perror("malloc");
		return EXIT_FAILURE;
	}


	for(i=0; i<loop; i++)
	{
		int ret=0;
		fill_buf(buffer, size);
		if(0 != (ret=write(fd, buffer, size*sizeof(char))))
		{
			printf("write %d data\n", ret);
		}
	}
	close(fd);
	return EXIT_SUCCESS;
}
