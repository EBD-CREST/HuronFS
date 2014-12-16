#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

int main(int argc, const char ** argv)
{
	int fd;
	char *buffer=NULL;
	if(-1 == (fd=open("/tmp/test", O_RDONLY)))
	{
		perror("open");
		return EXIT_FAILURE;
	}
	if(NULL == (buffer=malloc(sizeof(char))))
	{
		perror("malloc");
		return EXIT_FAILURE;
	}
	if(-1 == read(fd, buffer, sizeof(char)))
	{
		perror("read");
		return EXIT_FAILURE;
	}
	close(fd);
	return EXIT_SUCCESS;
}
