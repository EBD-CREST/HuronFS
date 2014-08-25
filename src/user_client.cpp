#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "include/User_Client.h"

int main(int argc, char**argv)
{
	if(2 > argc)
	{
		fprintf(stderr, "too few parameters\n"); 
		exit(0); 
	}
	try
	{
		User_Client client(std::string(argv[1]),0);
		char path[]="bin/testfile";
		ssize_t fd=client.iob_open(path, O_RDONLY);
		struct file_stat stat;
		client.iob_fstat(fd, stat);
		char *buffer=new char[stat.file_size+1];
		memset(buffer, 0, stat.file_size+1);
		client.iob_read(fd, buffer, stat.file_size);
		char buff[]="test is not a test file, you can print me out!";
		printf("%s", buffer);
		ssize_t fdwd=client.iob_open("test", O_WRONLY);
		client.iob_write(fdwd, buff, strlen(buff)); 
		client.iob_close(fdwd);
		client.iob_close(fd);
		delete buffer;
	}
	catch(std::runtime_error &e)
	{
		fprintf(stderr, "%s\n",e.what());
	}
	return 0;
}

