#ifndef _CBB_STDIO_H_

#define _CBB_STDIO_H__

#include <stdio.h>

extern const char * mount_point;

extern "C"
{
	int fclose(FILE* stream);
	int fflush(FILE* stream);
	FILE* fopen(const char* path, const char* mode);
	FILE* freopen(const char* path, const char* mode, FILE* stream);
	void setbuf(FILE* stream, char* buf);
	int setvbuf(FILE* stream, char* buf, int type, size_t size);

	/*int fprintf(FILE* stream, const char* format, ...);
	int fscanf(FILE* stream, const char* format, ...);
	int vfprintf(FILE* stream, const char* format, va_list ap);
	int vfscanf(FILE* stream, const char* format, va_list ap);*/

	int fgetc(FILE* stream);
	char* fgets(char* s, int n, FILE* stream);
	int fputc(int c, FILE* stream);
	int fputs(const char* s, FILE* stream);
	int ungetc(int c, FILE* stream);

	size_t fread(void* ptr, size_t size, size_t nitems, FILE* stream);
	size_t fwrite(const void* ptr, size_t size, size_t nitems, FILE* stream);

	int fgetpos(FILE* stream, fpos_t *pos);
	int fseek(FILE* stream, long offset, int whence);
	int fsetpos(FILE* stream, const fpos_t* pos);
	long ftell(FILE* stream);
	void rewind(FILE* stream);

	void clearerr(FILE* stream);
	int feof(FILE* stream)throw();
	int ferror(FILE* stream)throw();

	int fseeko(FILE* stream, off_t offset, int whence);
	off_t ftello(FILE* stream);
	int fileno(FILE* stream)throw();
}

#endif
