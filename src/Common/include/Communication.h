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
#include <exception>

#include "CBB_internal.h"
#include "CBB_const.h"

//API declearation
template<class T> inline size_t Recv(int sockfd, T& buffer)throw(std::runtime_error);

template<class T> inline size_t Send(int sockfd, const T& buffer)throw(std::runtime_error);

template<class T> inline size_t Recvv(int sockfd, T** buffer)throw(std::runtime_error);

template<class T> inline size_t Recvv_pre_alloc(int sockfd, T* buffer, size_t length)throw(std::runtime_error);

template<class T> inline size_t Sendv(int sockfd, const T* buffer, size_t count)throw(std::runtime_error);

template<class T> inline size_t Sendv_pre_alloc(int sockfd, const T* buffer, size_t count)throw(std::runtime_error);

template<class T> inline size_t Send_flush(int sockfd, const T& buffer)throw(std::runtime_error);

template<class T> inline size_t Sendv_flush(int sockfd, const T* buffer, size_t count)throw(std::runtime_error);

template<class T> inline size_t Sendv_pre_alloc_flush(int sockfd, const T* buffer, size_t count)throw(std::runtime_error);

template<class T> size_t Do_recv(int sockfd, T* buffer, size_t count, int flag)throw(std::runtime_error);

template<class T> size_t Do_send(int sockfd, const T* buffer, size_t count, int flag)throw(std::runtime_error);

//implementation

template<class T> inline size_t Recv(int sockfd, T& buffer)throw(std::runtime_error)

{
	return Do_recv<T>(sockfd, &buffer, 1, MSG_WAITALL);
}

template<class T> inline size_t Send(int sockfd, const T& buffer)throw(std::runtime_error)

{
	return Do_send<T>(sockfd, &buffer, 1, MSG_MORE|MSG_NOSIGNAL);
}

template<class T> inline size_t Recvv(int sockfd, T** buffer)throw(std::runtime_error)

{
	size_t length;
	//Recv(sockfd, length);
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

template<class T> inline size_t Sendv(int sockfd, const T* buffer, size_t count)throw(std::runtime_error)

{
	//Send(sockfd, count);
	return Do_send<T>(sockfd, buffer, count, MSG_MORE|MSG_NOSIGNAL);
}

template<class T> inline size_t Recvv_pre_alloc(int sockfd, T* buffer, size_t count)throw(std::runtime_error)

{
	return Do_recv<T>(sockfd, buffer, count, MSG_WAITALL);
}

template<class T> inline size_t Sendv_pre_alloc(int sockfd, const T* buffer, size_t count)throw(std::runtime_error)

{
	return Do_send<T>(sockfd, buffer, count, MSG_MORE|MSG_NOSIGNAL);
}
template<class T> inline size_t Send_flush(int sockfd, const T& buffer)throw(std::runtime_error)

{
	return Do_send<T>(sockfd, &buffer, 1, MSG_DONTWAIT|MSG_NOSIGNAL);
}

template<class T> inline size_t Sendv_flush(int sockfd, const T* buffer, size_t count)throw(std::runtime_error)

{
	//Send(sockfd, count);
	return Do_send<T>(sockfd, buffer, count, MSG_DONTWAIT|MSG_NOSIGNAL);
}

template<class T> inline size_t Sendv_pre_alloc_flush(int sockfd, const T* buffer, size_t count)throw(std::runtime_error)

{
	return Do_send<T>(sockfd, buffer, count, MSG_DONTWAIT|MSG_NOSIGNAL);
}

template<class T> size_t Do_recv(int sockfd, T* buffer, size_t count, int flag)throw(std::runtime_error)
{
	size_t length = sizeof(T)*count;
	ssize_t ret = 0;
	char *buffer_tmp = reinterpret_cast<char *>(buffer);

	if(0 == length)
	{
		return 0;
	}
	while(0 != length && 0 != (ret=recv(sockfd, buffer_tmp, length, flag)))
	{
		if(-1 == ret)
		{
			if(EINTR == errno || EAGAIN == errno)
			{
				perror("do_send");
				continue;
			}
			perror("do_recv");
			break;
		}
		buffer_tmp += ret;
		length -= ret;
	}
	if(0 == count*sizeof(T)-length)
	{
		_DEBUG("close socket %d\n", sockfd);
		close(sockfd);
		throw std::runtime_error("socket closed\n");
	}
	return count*sizeof(T)-length;
}

template<class T> size_t Do_send(int sockfd, const T* buffer, size_t count, int flag)throw(std::runtime_error)
{
	size_t length = sizeof(T)*count;
	ssize_t ret = 0;
	const char *buffer_tmp = reinterpret_cast<const char *>(buffer);

	if(0 == length)
	{
		return 0;
	}
	while(0 != length && 0 != (ret=send(sockfd, buffer_tmp, length, flag)))
	{
		if(-1 == ret)
		{
			if(EINTR == errno || EAGAIN == errno)
			{
				perror("do_send");
				continue;
			}
			perror("do_send");
			break;
		}
		buffer_tmp += ret;
		length -= ret;
	}
	if(0 == count*sizeof(T)-length)
	{
		_DEBUG("close socket %d\n", sockfd);
		close(sockfd);
		throw std::runtime_error("socket closed\n");
	}
	return count*sizeof(T)-length;
}
#endif
