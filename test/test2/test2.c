#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

//access to a remote file
int main(int argc, const char ** argv)
{
	int fd;
	char buffer;
	if(-1 == (fd=open("/home/xtq/test1", O_RDWR)))
	{
		perror("open");
		return EXIT_FAILURE;
	}
	if(-1 == read(fd, &buffer, sizeof(char)))
	{
		perror("read");
		return EXIT_FAILURE;
	}
	printf("%c\n", buffer);
	buffer='a';
	if(-1 == write(fd, &buffer, sizeof(char)))
	{
		perror("write");
		return EXIT_FAILURE;
	}
	flush(fd);
	close(fd);
	return EXIT_SUCCESS;
}
