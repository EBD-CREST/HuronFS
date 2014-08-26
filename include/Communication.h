/*
 * head file defines function used in tcp/ip communication
 */

#ifndef COMMINCATION_H_
#define COMMINCATION_H_

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

//API declearation
template<class T> size_t Recv(int sockfd, T& buffer);

template<class T> size_t Send(int sockfd, const T& buffer);

template<class T> size_t Recvv(int sockfd, T** buffer);

template<class T> size_t Recvv_pre_alloc(int sockfd, T* buffer, size_t length);

template<class T> size_t Sendv(int sockfd, const T* buffer, size_t count);

template<class T> size_t Sendv_pre_alloc(int sockfd, const T* buffer, size_t count);

//implementation

template<class T> size_t Recv(int sockfd, T& buffer)
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

template<class T> size_t Send(int sockfd, const T& buffer)
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

template<class T> size_t Recvv(int sockfd, T **buffer)
{
	size_t length;
	ssize_t ret = 0;
	Recv(sockfd, length); 
	
	*buffer=new T[length+1]; 
	(*buffer)[length]=0;
	char *buffer_tmp = reinterpret_cast<char *>(*buffer);
	if(NULL  ==  *buffer)
	{
		return 0; 
	}
	
	struct iovec iov; 
	iov.iov_base=reinterpret_cast<void*>(buffer_tmp); 
	iov.iov_len=length; 

	while(0 != length && 0 != (ret=readv(sockfd, &iov, 1)))
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
		iov.iov_base=reinterpret_cast<void*>(buffer_tmp); 
		iov.iov_len=length; 
	}
	return length;
}

template<class T> size_t Recvv_pre_alloc(int sockfd, T *buffer, size_t length)
{
	ssize_t ret = 0;
	
	char *buffer_tmp = reinterpret_cast<char *>(buffer);
	struct iovec iov; 
	iov.iov_base=reinterpret_cast<void*>(buffer_tmp); 
	iov.iov_len=length; 

	while(0 != length && 0 != (ret=readv(sockfd, &iov, 1)))
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
		iov.iov_base=reinterpret_cast<void*>(buffer_tmp); 
		iov.iov_len=length; 
	}
	return length;
}

template<class T> size_t Sendv(int sockfd,const T* buffer, size_t count)
{
	size_t length = count*sizeof(T);
	ssize_t ret = 0;
	char *buffer_tmp = const_cast<char*>(reinterpret_cast<const char *>(buffer));
	struct iovec iov; 
	iov.iov_base=const_cast<void *>(reinterpret_cast<const void*>(buffer)); 
	iov.iov_len=length; 

	Send(sockfd, length); 
	while(0 != length && 0 != (ret=writev(sockfd, &iov, 1)))
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
		iov.iov_base=reinterpret_cast<void*>(buffer_tmp); 
		iov.iov_len=length; 
	}
	return count*sizeof(T)-length;
}

template<class T> size_t Sendv_pre_alloc(int sockfd,const T* buffer, size_t count)
{
	size_t length = count*sizeof(T);
	ssize_t ret = 0;
	char *buffer_tmp = const_cast<char*>(reinterpret_cast<const char *>(buffer));
	struct iovec iov; 
	iov.iov_base=const_cast<void *>(reinterpret_cast<const void*>(buffer)); 
	iov.iov_len=length; 
	printf("length=%lu\n",length);
	puts("before sending");
	while(0 != length && 0 != (ret=writev(sockfd, &iov, 1)))
	{
		if(-1 == ret)
		{
			if(EINVAL == errno)
			{
				continue; 
			}
			break; 
		}
		printf("ret=%lu\n",ret);
		buffer_tmp += ret; 
		length -= ret; 
		iov.iov_base=reinterpret_cast<void*>(buffer_tmp); 
		iov.iov_len=length; 
	}
	puts("after sending\n");
	return count*sizeof(T)-length;
}

#endif
