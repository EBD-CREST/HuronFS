#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <error.h>
#include <limits.h>

#include "CBB.h"
#include "CBB_stdio.h"
#include "CBB_internal.h"
#include "CBB_stream.h"

extern CBB_stream client;

extern "C" FILE *CBB_WRAP(fopen)(const char* path, const char* mode)
{
	CBB_FUNC_P(FILE*, fopen, (const char* path, const char *mode));
	std::string formatted_path;
	CBB::_format_path(path, formatted_path);
	if(CBB::_interpret_path(formatted_path.c_str()))
	{
		std::string relative_path;
		CBB::_get_relative_path(formatted_path, relative_path);
		_LOG("CBB open path=%s,mode=%s\n", formatted_path.c_str(), mode);
		return client._open_stream(relative_path.c_str(), mode);
	}
	else
	{
		MAP_BACK(fopen);
		return CBB_REAL(fopen)(path, mode);
	}
}

extern "C" FILE *CBB_WRAP(freopen)(const char* path, const char* mode, FILE* stream)
{
	CBB_FUNC_P(FILE*, freopen, (const char* path, const char *mode, FILE* stream));
	if(NULL != stream && client._interpret_stream(stream))
	{
		client._close_stream(stream);
		_LOG("CBB freopen path=%s, mode=%s\n", path, mode);
		return client._open_stream(path, mode);
	}
	else
	{
		MAP_BACK(freopen);
		return CBB_REAL(freopen)(path, mode, stream);
	}
}
extern "C" int CBB_WRAP(fclose)(FILE *stream)
{
	CBB_FUNC_P(int, fclose, (FILE *stream));
	if(client._interpret_stream(stream))
	{
		_LOG("CBB fclose\n");
		client._flush_stream(stream);
		return client._close_stream(stream);
	}
	else
	{
		MAP_BACK(fclose);
		return CBB_REAL(fclose)(stream);
	}
}

extern "C" int CBB_WRAP(fflush)(FILE *stream)
{
	CBB_FUNC_P(int, fflush, (FILE *stream));
	if(client._interpret_stream(stream))
	{

		_LOG("CBB fflush\n");
		return client._flush_stream(stream);
	}
	else
	{
		MAP_BACK(fflush);
		return CBB_REAL(fflush)(stream);
	}
}

extern "C" void CBB_WRAP(setbuf)(FILE * stream, char * buf)
{
	CBB_FUNC_P(void*, setbuf, (FILE* stream, char* buf));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB setbuf\n");
		client._freebuf_stream(stream);
		client._setbuf_stream(stream, buf);
	}
	else
	{
		MAP_BACK(setbuf);
		CBB_REAL(setbuf)(stream, buf);
	}
}

extern "C" int CBB_WRAP(setvbuf)(FILE* stream, char* buf, int type, size_t size)
{
	CBB_FUNC_P(int, setvbuf, (FILE* stream, char* buf, int type, size_t size));
	if(true)
	{

		_ERROR("do not support yet\n");
	//	return -1;
	//}
	//else
	//{
		MAP_BACK(setvbuf);
		return CBB_REAL(setvbuf)(stream, buf, type, size);
	}
}

extern "C" int CBB_WRAP(fprintf)(FILE *stream, const char * format, ...)
{
	va_list ap;
	va_start(ap, format);
	int ret=CBB_WRAP(vfprintf)(stream, format, ap);
	va_end(ap);
	return ret;
}

extern "C" int CBB_WRAP(fscanf)(FILE *stream, const char * format, ...)
{
	va_list ap;
	va_start(ap, format);
	int ret=CBB_WRAP(vfscanf)(stream, format, ap);
	va_end(ap);
	return ret;
}

extern "C" int CBB_WRAP(vfprintf)(FILE *stream, const char * format,va_list ap) 
{
	CBB_FUNC_P(int, vfprintf, (FILE *stream, const char *format, va_list ap)); 
	if(true)
	{

		_LOG("do not support yet\n");
	//	return EOF;
	//}
	//else
	//{
		MAP_BACK(vfprintf);
		return CBB_REAL(vfprintf)(stream, format, ap);
	}
}

extern "C" int CBB_WRAP(vfscanf)(FILE *stream, const char * format, va_list ap)
{
	CBB_FUNC_P(int, vfscanf, (FILE *stream, const char *format, va_list ap));

	if(true)
	{

		_LOG("do not support yet\n");
	//	return EOF;
	//}
	//else
	//{
		MAP_BACK(vfscanf);
		return CBB_REAL(vfscanf)(stream, format, ap);
	}
}

extern "C" int CBB_WRAP(fgetc)(FILE *stream)
{
	CBB_FUNC_P(int, fgetc, (FILE *stream));

	int c;
	if(client._interpret_stream(stream))
	{	
		//_LOG("CBB fgetc\n");
		size_t ret=client._read_stream(stream, &c, sizeof(char));
		if(sizeof(char) != ret)
		{
			return EOF;
		}
		else
		{
			//#if DEBUG
			//		if(EOF != c)
			//		{
			printf("%c", c);
			//		}
			//#endif
			return c;
		}
	}
	else
	{
		MAP_BACK(fgetc);
		c=CBB_REAL(fgetc)(stream);
#if 0
//#ifdef DEBUG
		if(EOF != c)
		{
			printf("%c\n", c);
		}
#endif
		return c;
	}
}

extern "C" char *CBB_WRAP(fgets)(char* s, int n, FILE* stream)
{
	CBB_FUNC_P(char*, fgets, (char* s, int n, FILE* stream));

	char* start=s;
	if(client._interpret_stream(stream))
	{
		_LOG("CBB fgets");
		while(--n)
		{
			client._read_stream(stream, s, sizeof(char));
			if('\n' == *s)
			{
				*s='\0';
				return start;
			}
			++s;
		}
		_LOG("%s\n",start);
		return start;
	}
	else
	{
		MAP_BACK(fgets);
		start=CBB_REAL(fgets)(s, n, stream);
//#ifdef DEBUG
#if 0
		if(NULL != start)
		{
			printf("%s\n",start);
		}
#endif
		return start;
	}
}

extern "C" int CBB_WRAP(fputc)(int c, FILE* stream)
{
	CBB_FUNC_P(int, fputc, (int c, FILE* stram));
	if(client._interpret_stream(stream))
	{
		//_LOG("CBB fputc\n");
		size_t ret=client._write_stream(stream, &c, sizeof(char));
		if(sizeof(char) != ret)
		{
			return EOF;
		}
		else
		{
			return c;
		}
	}
	else
	{
		MAP_BACK(fputc);
		return CBB_REAL(fputc)(c, stream);
	}
}

extern "C" int CBB_WRAP(fputs)(const char* s, FILE *stream)
{
	CBB_FUNC_P(int, fputs, (const char* s, FILE *stream));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB fputs\n");
		int len=strlen(s);
		_LOG("string=%s\n", s);
		client._write_stream(stream, s, len*sizeof(char));
		return 1;
	}
	else
	{
		MAP_BACK(fputs);
		return CBB_REAL(fputs)(s, stream);
	}
}

extern "C" int CBB_WRAP(ungetc)(int c, FILE *stream)
{
	CBB_FUNC_P(int, ungetc, (int c, FILE *stream));

	if(true)
	{
		_ERROR("do not support yet\n");
	//	return EOF;
	//}
	//else
	//{
		MAP_BACK(ungetc);
		return CBB_REAL(ungetc)(c, stream);
	}
}

extern "C" size_t CBB_WRAP(fread)(void *ptr, size_t size, size_t nitems, FILE *stream)
{
	CBB_FUNC_P(size_t, fread, (void *ptr, size_t size, size_t nitems, FILE *stream));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB fread");
		_LOG("file no=%d\n", fileno(stream));
		size_t IO_size=size*nitems;
		return client._read_stream(stream, ptr, IO_size);
	}
	else
	{
		MAP_BACK(fread);
		return CBB_REAL(fread)(ptr, size, nitems, stream);
	}
}

extern "C" size_t CBB_WRAP(fwrite)(const void *ptr, size_t size, size_t nitems, FILE *stream)
{
	CBB_FUNC_P(size_t, fwrite, (const void *ptr, size_t size, size_t nitems, FILE *stream));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB fwrite");
		_LOG("file no=%d\n", fileno(stream));
		size_t IO_size=size*nitems;
		return client._write_stream(stream, ptr, IO_size);
	}
	else
	{
		MAP_BACK(fwrite);
		return CBB_REAL(fwrite)(ptr, size, nitems, stream);
	}
}

extern "C" int CBB_WRAP(fgetpos)(FILE *stream, fpos_t* pos)
{
	CBB_FUNC_P(int, fgetpos, (FILE *stream, fpos_t* pos));

	if(client._interpret_stream(stream))
	{
		_ERROR("do not support yet\n");
		return -1;//client._getpos(static_cast<stream_info_t*>(stream));
	}
	else
	{
		MAP_BACK(fgetpos);
		return CBB_REAL(fgetpos)(stream, pos);
	}
}

extern "C" int CBB_WRAP(fseek)(FILE *stream, long offset, int whence)
{
	CBB_FUNC_P(int, fseek, (FILE *stream, long offset, int whence));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB fseek\n");
		_LOG("offset %ld\n", offset);
		return client._seek_stream(stream, offset, whence);
	}
	else
	{
		MAP_BACK(fseek);
		return CBB_REAL(fseek)(stream, offset, whence);
	}
}

extern "C" int CBB_WRAP(fsetpos)(FILE *stream, const fpos_t* pos)
{
	CBB_FUNC_P(int, fsetpos, (FILE *stream, const fpos_t* pos));

	//if(client._interpret_stream(stream))
	if(true)
	{
		_ERROR("do not support yet\n");

		//return -1;
	//}
	//else
	//{
		MAP_BACK(fsetpos);
		return CBB_REAL(fsetpos)(stream, pos);
	}
}

extern "C" long CBB_WRAP(ftell)(FILE *stream)
{
	CBB_FUNC_P(long, ftell, (FILE *stream));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB ftell\n");
		long pos=client._tell_stream(stream);
		_LOG("CBB pos=%ld\n", pos);
		return pos;
	}
	else
	{
		MAP_BACK(ftell);
		return CBB_REAL(ftell)(stream);
	}
}

extern "C" void CBB_WRAP(rewind)(FILE *stream)
{
	CBB_FUNC_P(void, rewind, (FILE *stream));
	if(client._interpret_stream(stream))
	{
		_LOG("CBB rewind\n");
		client._seek_stream(stream, 0, SEEK_SET);
	}
	else
	{
		MAP_BACK(rewind);
		CBB_REAL(rewind)(stream);
	}
}

extern "C" void CBB_WRAP(clearerr)(FILE *stream)
{
	CBB_FUNC_P(void, clearerr, (FILE *stream));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB clearerr\n");
		client._clearerr_stream(stream);
	}
	else
	{
		MAP_BACK(clearerr);
		CBB_REAL(clearerr)(stream);
	}
}

extern "C" int CBB_WRAP(feof)(FILE *stream)
{
	CBB_FUNC_P(int, feof, (FILE *stream));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB feof\n");
		return client._eof_stream(stream);
	}
	else
	{
		MAP_BACK(feof);
		return CBB_REAL(feof)(stream);
	}
}

extern "C" int CBB_WRAP(ferror)(FILE *stream)
{
	CBB_FUNC_P(int, ferror, (FILE *stream));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB ferror\n");
		return client._error_stream(stream);
	}
	else
	{
		MAP_BACK(ferror);
		return CBB_REAL(ferror)(stream);
	}
}

extern "C" int CBB_WRAP(fseeko)(FILE *stream, off_t offset, int whence)
{
	CBB_FUNC_P(int, fseeko, (FILE *stream, off_t offset, int whence));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB fseeko\n");
		return client._seek_stream(stream, offset, whence);
	}
	else
	{
		MAP_BACK(fseeko);
		return CBB_REAL(fseeko)(stream, offset, whence);
	}
}

extern "C" off_t CBB_WRAP(ftello)(FILE *stream)
{
	CBB_FUNC_P(off_t, ftello, (FILE *stream));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB ftello\n");
		return client._tell_stream(stream);
	}
	else
	{
		MAP_BACK(ftello);
		return CBB_REAL(ftello)(stream);
	}
}

extern "C" int CBB_WRAP(fileno)(FILE *stream)
{
	CBB_FUNC_P(int, fileno, (FILE *stream));

	if(client._interpret_stream(stream))
	{
		_LOG("CBB fileno\n");
		int fd=client._fileno_stream(stream);
		_LOG("fd=%d\n", fd);
		return fd;
	}
	else
	{
		MAP_BACK(fileno);
		return CBB_REAL(fileno)(stream);
	}
}

