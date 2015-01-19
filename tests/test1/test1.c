#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

//access to a local file
int main(int argc, const char ** argv)
{
	int fd;
	char buffer;
	if(-1 == (fd=open("/tmp/test", O_RDONLY)))
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
	close(fd);
	return EXIT_SUCCESS;
}
