#include <stdio.h>
#include <stdlib.h>

//test buffer IO API
//test function:
//fopen, fread, fwrite, fflush, fclose


int main(int argc, char ** argv)
{
	FILE *fp=NULL;
	int buffer;
	int count=0, i=0;
	char *data="this is a test text";
	if(NULL == (fp=fopen("../../../test1", "r")))
	{
		perror("fopen");
		return EXIT_FAILURE;
	}
	/*if(NULL == (buffer=malloc(sizeof(char))))
	{
		perror("malloc");
		return EXIT_FAILURE;
	}*/

	while(!feof(fp))
	{
		if(-1 == (buffer=fgetc(fp)))
		{
			perror("fgetc");
			return EXIT_FAILURE;
		}
		printf("%c\n", buffer);
		++count;
	}
	rewind(fp);
	for(i=0; i<count;++i)
	{
		fputc(data[i], fp);
	}
	
	return EXIT_SUCCESS;
}
