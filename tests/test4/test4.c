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
