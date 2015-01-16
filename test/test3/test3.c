#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>

//access to a remote file
//test function:
//open, read, write, flush, close
int main(int argc, const char ** argv)
{
	int fd1, fd2;
	char buffer;
	if(-1 == (fd1=open("../../../test1", O_RDWR)))
	{
		perror("open");
		return EXIT_FAILURE;
	}
	if(-1 == (fd2=open("../../../test2",O_RDONLY)))
	{
		perror("open");
		return EXIT_FAILURE;
	}

	if(-1 == read(fd1, &buffer, sizeof(char)))
	{
		perror("read");
		return EXIT_FAILURE;
	}
	printf("%c\n", buffer);
	if(-1 == read(fd2, &buffer, sizeof(char)))
	{
		perror("read");
		return EXIT_FAILURE;
	}

	if(-1 == lseek(fd1, 0, SEEK_END))
	{
		perror("lseek");
		return EXIT_FAILURE;
	}

	if(-1 == write(fd1, &buffer, sizeof(char)))
	{
		perror("write");
		return EXIT_FAILURE;
	}
	close(fd1);
	close(fd2);
	return EXIT_SUCCESS;
}
