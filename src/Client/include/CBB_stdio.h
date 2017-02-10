/*
 * Copyright (c) 2017, Tokyo Institute of Technology
 * Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
 * All rights reserved. 
 * 
 * This file is part of HuronFS.
 * 
 * Please also read the file "LICENSE" included in this package for 
 * Our Notice and GNU Lesser General Public License.
 * 
 * This program is free software; you can redistribute it and/or modify it under the 
 * terms of the GNU General Public License (as published by the Free Software 
 * Foundation) version 2.1 dated February 1999. 
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */


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
