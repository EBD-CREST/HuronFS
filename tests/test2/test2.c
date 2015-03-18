#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <unistd.h>

//access to a remote file
//test function:
//open, read, write, flush, close

const size_t READ_SIZE=34;
int main(int argc, const char ** argv)
{
	int fd1, fd2;
	char *buffer=NULL;
	int ret=0;
	char *data="append this content after test1";
	char file_path1[PATH_MAX], file_path2[PATH_MAX];
	const char *mount_point=getenv("CBB_CLIENT_MOUNT_POINT");
	struct stat filestat;

	if(NULL == mount_point)
	{
		fprintf(stderr, "please set CBB_CLIENT_MOUNT_POINT");
		return EXIT_FAILURE;
	}

	sprintf(file_path1, "%s%s", mount_point, "/montage/solvers/gxp_make/p2mass-atlas-990214n-j1200256_area.fits");
	//sprintf(file_path1, "%s%s", mount_point, "/test1");
	sprintf(file_path2, "%s%s", mount_point, "/test2");

	if(-1 == (fd1=open(file_path1, O_RDWR|O_CREAT|O_TRUNC, 0666)))
	{
		perror("open");
		return EXIT_FAILURE;
	}
/*	if(-1 == (fd2=open(file_path2,O_RDONLY)))
	{
		perror("open");
		return EXIT_FAILURE;
	}*/
	if(NULL == (buffer=malloc(4096*sizeof(char))))
	{
		perror("malloc");
		return EXIT_FAILURE;
	}
	if(-1 == fstat(fd1, &filestat))
	{
		perror("fstat");
		return EXIT_FAILURE;
	}
	if(-1 == write(fd1, buffer, 2880*sizeof(char)))
	{
		perror("write");
		return EXIT_FAILURE;
	}
	if(-1 == lseek(fd1, 0, SEEK_SET))
	{
		perror("lseek");
		return EXIT_FAILURE;
	}
	if(-1 == lseek(fd1, 0, SEEK_SET))
	{
		perror("lseek");
		return EXIT_FAILURE;
	}

	if(-1 == (ret=read(fd1, buffer, 4096*sizeof(char))))
	{
		perror("read");
		return EXIT_FAILURE;
	}
	printf("ret=%d\n",ret);	
	buffer[34]=0;
	printf("%s\n", buffer);
	if(-1 == read(fd2, buffer, sizeof(char)))
	{
		perror("read");
		return EXIT_FAILURE;
	}

	if(-1 == lseek(fd1, 35, SEEK_SET))
	{
		perror("lseek");
		return EXIT_FAILURE;
	}

	if(-1 == write(fd1, data, strlen(data)))
	{
		perror("write");
		return EXIT_FAILURE;
	}
	close(fd1);
	close(fd2);
	return EXIT_SUCCESS;
}
