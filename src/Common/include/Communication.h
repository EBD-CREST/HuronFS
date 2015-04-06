/*
 * head file defines function used in tcp/ip communication
 */

#ifndef COMMINCATION_H_
#define COMMINCATION_H_

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "CBB_internal.h"

//API declearation
template<class T> inline size_t Recv(int sockfd, T& buffer);

template<class T> inline size_t Send(int sockfd, const T& buffer);

template<class T> inline size_t Recvv(int sockfd, T** buffer);

template<class T> inline size_t Recvv_pre_alloc(int sockfd, T* buffer, size_t length);

template<class T> inline size_t Sendv(int sockfd, const T* buffer, size_t count);

template<class T> inline size_t Sendv_pre_alloc(int sockfd, const T* buffer, size_t count);

template<class T> inline size_t Send_flush(int sockfd, const T& buffer);

template<class T> inline size_t Sendv_flush(int sockfd, const T* buffer, size_t count);

template<class T> inline size_t Sendv_pre_alloc_flush(int sockfd, const T* buffer, size_t count);

template<class T> size_t Do_recv(int sockfd, T* buffer, size_t count, int flag);

template<class T> size_t Do_send(int sockfd, const T* buffer, size_t count, int flag);

//implementation

template<class T> inline size_t Recv(int sockfd, T& buffer)
{
	return Do_recv<T>(sockfd, &buffer, 1, MSG_WAITALL);
}

template<class T> inline size_t Send(int sockfd, const T& buffer)
{
	return Do_send<T>(sockfd, &buffer, 1, MSG_MORE|MSG_NOSIGNAL);
}

template<class T> inline size_t Recvv(int sockfd, T** buffer)
{
	size_t length;
	Recv(sockfd, length);
	if(NULL == (*buffer=new char[length+1]))
	{
		perror("new");
		return 0;
	}
	else
	{
		(*buffer)[length]=0;
		return Do_recv<T>(sockfd, *buffer, length, MSG_WAITALL);
	}
}

template<class T> inline size_t Sendv(int sockfd, const T* buffer, size_t count)
{
	Send(sockfd, count);
	return Do_send<T>(sockfd, buffer, count, MSG_MORE|MSG_NOSIGNAL);
}

template<class T> inline size_t Recvv_pre_alloc(int sockfd, T* buffer, size_t count)
{
	return Do_recv<T>(sockfd, buffer, count, MSG_WAITALL);
}

template<class T> inline size_t Sendv_pre_alloc(int sockfd, const T* buffer, size_t count)
{
	return Do_send<T>(sockfd, buffer, count, MSG_MORE|MSG_NOSIGNAL);
}
template<class T> inline size_t Send_flush(int sockfd, const T& buffer)
{
	return Do_send<T>(sockfd, &buffer, 1, MSG_DONTWAIT|MSG_NOSIGNAL);
}

template<class T> inline size_t Sendv_flush(int sockfd, const T* buffer, size_t count)
{
	Send(sockfd, count);
	return Do_send<T>(sockfd, buffer, count, MSG_DONTWAIT|MSG_NOSIGNAL);
}

template<class T> inline size_t Sendv_pre_alloc_flush(int sockfd, const T* buffer, size_t count)
{
	return Do_send<T>(sockfd, buffer, count, MSG_DONTWAIT|MSG_NOSIGNAL);
}

template<class T> size_t Do_recv(int sockfd, T* buffer, size_t count, int flag)
{
	size_t length = sizeof(T)*count;
	ssize_t ret = 0;
	char *buffer_tmp = reinterpret_cast<char *>(buffer);

	while(0 != length && 0 != (ret=recv(sockfd, buffer_tmp, length, flag)))
	{
		if(-1 == ret)
		{
			if(EINVAL == errno || EAGAIN == errno)
			{
				continue;
			}
			perror("do_recv");
			break;
		}
		buffer_tmp += ret;
		length -= ret;
	}
	return count*sizeof(T)-length;
}

template<class T> size_t Do_send(int sockfd, const T* buffer, size_t count, int flag)
{
	size_t length = sizeof(T)*count;
	ssize_t ret = 0;
	const char *buffer_tmp = reinterpret_cast<const char *>(buffer);

	while(0 != length && 0 != (ret=send(sockfd, buffer_tmp, length, flag)))
	{
		if(-1 == ret)
		{
			if(EINVAL == errno || EAGAIN == errno)
			{
				continue;
			}
			perror("do_send");
			break;
		}
		buffer_tmp += ret;
		length -= ret;
	}
	return count*sizeof(T)-length;
}

inline int Send_attr(int socket, const struct stat* file_stat)
{
	Send(socket, file_stat->st_mode);    /* protection */
	Send(socket, file_stat->st_uid);     /* user ID of owner */
	Send(socket, file_stat->st_gid);     /* group ID of owner */
	Send(socket, file_stat->st_size);    /* total size, in bytes */
	Send(socket, file_stat->st_atime);   /* time of last access */
	Send(socket, file_stat->st_mtime);   /* time of last modification */
	Send_flush(socket, file_stat->st_ctime);   /* time of last status change */
	return SUCCESS;
}

inline int Recv_attr(int socket, struct stat* file_stat)
{
	Recv(socket, file_stat->st_mode);    /* protection */
	Recv(socket, file_stat->st_uid);     /* user ID of owner */
	Recv(socket, file_stat->st_gid);     /* group ID of owner */
	Recv(socket, file_stat->st_size);    /* total size, in bytes */
	Recv(socket, file_stat->st_atime);   /* time of last access */
	Recv(socket, file_stat->st_mtime);   /* time of last modification */
	Recv(socket, file_stat->st_ctime);   /* time of last status change */
	return SUCCESS;
}
#endif
