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

#ifndef _CBB_POSIX_H_

#define _CBB_POSIX_H_

extern const char * mount_point;

extern "C" 
{
	int open(const char *path, int flag, ...); 
	int open64(const char *path, int flag, ...); 
	ssize_t read(int fd, void *buffer, size_t size);
	ssize_t read64(int fd, void *buffer, size_t size);
	ssize_t write(int fd, const void *buffer, size_t size);
	ssize_t write64(int fd, const void *buffer, size_t size);
	int close(int fd);
	int flush(int fd);

	int creat(const char* path, mode_t mode);
	int creat64(const char* path, mode_t mode);
	
	ssize_t readv(int fd, const struct iovec* iov, int iovcnt);
	ssize_t writev(int fd, const struct iovec* iov, int iovcnt);
	ssize_t pread(int fd, void* buf, size_t count, off_t offset);
	ssize_t pread64(int fd, void* buf, size_t count, off64_t offset);
	ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset);
	ssize_t pwrite64(int fd, const void* buf, size_t count, off64_t offset);
	int posix_fadvise(int fd, off_t offset, off_t len, int advice);
	off_t lseek(int fd, off_t offset, int whence);
	off64_t lseek64(int fd, off64_t offset, int whence);

	int stat(const char* path, struct stat* buf);
	int fstat(int fd, struct stat* buf);
	int lstat(const char *path, struct stat* buf);

	int ftruncate(int fd, off_t length)throw();
	int fsync(int fd);
	int fdatasync(int fd);
	int flock(int fd, int operation);
	void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset);
	void* mmap64(void* addr, size_t length, int prot, int flags, int fd, off64_t offset);
	int munmap(void* addr, size_t length);
	int msync(void* addr, size_t length, int flags);
}

#endif
