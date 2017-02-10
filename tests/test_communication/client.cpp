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

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include <Communication.h>

#define PORT 9100
#define HELLO_WORLD_CLIENT_PORT 9101
#define BUFFER_SIZE 1024

//size_t data[INNER_LOOP];
inline double TIME(const struct timeval *st, const struct timeval *et)
{
	return et->tv_sec-st->tv_sec+(et->tv_usec-st->tv_usec)/1000000.0;
}

int main(int argc, char **argv)
{
	struct timeval st, et;
	if(2 > argc)
	{
		fprintf(stderr, "too few peremeter\n");
		exit(1);
	}
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htons(INADDR_ANY);
	client_addr.sin_port = htons(0);

	int client_socket = socket(PF_INET, SOCK_STREAM, 0);
	if( client_socket < 0)
	{
		fprintf(stderr, "Create Socket Failed\n");
		exit(1);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT);
	if( 0 == inet_aton(argv[1],&server_addr.sin_addr))
	{
		fprintf(stderr, "Server IP Address Error\n");
		exit(0);
	}
	socklen_t server_addr_length = sizeof(server_addr);

	if(connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) != 0)
	{
		perror("Connect error");
		exit(0);
	}
	int i=0;
	/*for(i=0;i<INNER_LOOP; ++i)
	{
		data[i]=i;
	}*/
/*	gettimeofday(&st, NULL);
	for(i=0;i<OUTER_LOOP;++i)
	{
		Sendv(client_socket, data, INNER_LOOP);
	}
	gettimeofday(&et, NULL);
	printf("merged, loop time=%d, time=%fs\n", OUTER_LOOP, TIME(&st, &et));*/
	gettimeofday(&st, NULL);
	int test_int=1;
	size_t test_size_t=2;
	const char *string="this is a test string";
	//char recv_string[1000];
	char *recv_string=NULL;
	size_t message_size=sizeof(test_int);
	message_size+=sizeof(test_size_t);
	message_size+=sizeof(size_t)+(strlen(string)+1)*sizeof(char);

	size_t size;
	Send(client_socket, message_size);
	Send(client_socket, test_int);
	Send_flush(client_socket, test_size_t);
	Sendv_flush(client_socket, string, strlen(string)+1);

	Recv(client_socket, size);
	//Recvv_pre_alloc(client_socket, recv_string, size);
	Recv(client_socket, test_int);
	Recv(client_socket, test_size_t);
	Recvv(client_socket, &recv_string);
	printf("size=%ld\ntest int %d\ntest size_t %lu\ntest string %s\n", size, test_int, test_size_t, recv_string);
	/*int j=0;
	for(j=0;j<OUTER_LOOP;++j)
	{
		//Sendv_pre_alloc(client_socket, data, INNER_LOOP);
		//Recvv_pre_alloc(client_socket, data, INNER_LOOP);
		for(i=0;i<INNER_LOOP;++i)
		{
			Send(client_socket, data[i]);
			//Sendv(client_socket, &data[i], 1);
		}
		printf("%d\n", j);
		for(i=0;i<INNER_LOOP;++i)
		{
			Recv(client_socket, data[i]);
			//Recv(client_socket, data[i]);
			//Recvv_pre_alloc(client_socket, &data[i], 1);
		}
	}*/

	gettimeofday(&et, NULL);
	//printf("loop time=%d, time=%fs\n", OUTER_LOOP, TIME(&st, &et));
	//while(1);
	close(client_socket);
	return 0;
}
