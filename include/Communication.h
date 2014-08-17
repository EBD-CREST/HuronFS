/*
 * head file defines function used in tcp/ip communication
 */

#ifndef COMMINCATION_H_
#define COMMINCATION_H_

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

//API declearation
template<class T> std::size_t Recv(int sockfd, T& buffer);

template<class T> std::size_t Send(int sockfd, const T& buffer);

template<class T> int Recvv(int sockfd, T* buffer, int count);

template<class T> int Sendv(int sockfd, const T* buffer, int count);

//implementation

template<class T> std::size_t Recv(int sockfd, T& buffer)
{
	size_t length = sizeof(buffer);
	ssize_t ret = 0;
	char *buffer_tmp = reinterpret_cast<char *>(&buffer);

	while(0 != length && 0 != (ret=read(sockfd, buffer_tmp, length)))
	{
		if(-1 == ret)
		{
			if(EINVAL == errno)
			{
				continue;
			}
			break;
		}
		buffer_tmp += ret;
		length -= ret;
	}
	return sizeof(buffer)-length;
}

template<class T> std::size_t Send(int sockfd, const T& buffer)
{
	size_t length = sizeof(buffer);
	ssize_t ret = 0;
	const char *buffer_tmp = reinterpret_cast<const char *>(&buffer);

	while(0 != length && 0 != (ret=write(sockfd, buffer_tmp, length)))
	{
		if(-1 == ret)
		{
			if(EINVAL == errno)
			{
				continue;
			}
			break;
		}
		buffer_tmp += ret;
		length -= ret;
	}
	return sizeof(buffer)-length;
}

template<class T> std::size_t Recvv(int sockfd, T* buffer, int count)
{
	int i=0;
	for(;i<count;++i)
	{
		if(sizeof(T) != Recv(sockfd, buffer[i]))
		{
			break;
		}
	}
	return i;
}

template<class T> std::size_t Sendv(int sockfd, const T* buffer, int count)
{
	int i=0;
	for(;i<count;++i)
	{
		if(sizeof(T) != Send(sockfd, buffer[i]))
		{
			break;
		}
	}
	return i;
}

#endif
