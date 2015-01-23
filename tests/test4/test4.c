#include <stdio.h>
#include <stdlib.h>

//test buffer IO API
//test function:
//fopen, fread, fwrite, fflush, fclose


int main(int argc, char ** argv)
{
	FILE *fp=NULL;
	char *buffer=NULL;
	int ret;
	if(NULL == (fp=fopen("../../../test1", "r")))
	{
		perror("fopen");
		return EXIT_FAILURE;
	}
	if(NULL == (buffer=malloc(sizeof(char))))
	{
		perror("malloc");
		return EXIT_FAILURE;
	}

	if(-1 == fread(buffer, sizeof(char), 1, fp))
	{
		perror("fread");
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
