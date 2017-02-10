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

//test buffer IO API
//test function:
//fopen, fread, fwrite, fflush, fclose


int main(int argc, char ** argv)
{
	FILE *fp_rd=NULL, *fp_wr=NULL;
	//int buffer;
	char *buffer=NULL;
	size_t size=104857600;
	if(argc < 3)
	{
		fprintf(stderr, "usage test4 [src] [des]\n");
		return EXIT_FAILURE;
	}
	if(NULL == (fp_rd=fopen(argv[1], "r")))
	{
		perror("fopen");
		return EXIT_FAILURE;
	}
	if(NULL == (buffer=malloc(size)))
	{
		perror("malloc");
		return EXIT_FAILURE;
	}

/*	if(NULL == (fp_wr=fopen(argv[2], "w")))
	{
		perror("fopen");
		return EXIT_FAILURE;
	}
	while(EOF != (buffer=fgetc(fp_rd)))
	{
		fputc(buffer, fp_wr);
	}*/

	fseek(fp_rd, 0, SEEK_SET);
	fread(buffer, size, sizeof(char), fp_rd);
	fclose(fp_rd);
//	fclose(fp_wr);
	return EXIT_SUCCESS;
}
