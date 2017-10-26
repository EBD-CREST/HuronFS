# Copyright (c) 2017, Tokyo Institute of Technology
# Written by Tianqi Xu, xu.t.aa@m.titech.ac.jp.
# All rights reserved. 
# 
# This file is part of HuronFS.
# 
# Please also read the file "LICENSE" included in this package for 
# Our Notice and GNU Lesser General Public License.
# 
# This program is free software; you can redistribute it and/or modify it under the 
# terms of the GNU General Public License (as published by the Free Software 
# Foundation) version 2.1 dated February 1999. 
# 
# This program is distributed in the hope that it will be useful, but WITHOUT ANY 
# WARRANTY; without even the IMPLIED WARRANTY OF MERCHANTABILITY or 
# FITNESS FOR A PARTICULAR PURPOSE. See the terms and conditions of the GNU 
# General Public License for more details. 
# 
# You should have received a copy of the GNU Lesser General Public License along 
# with this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
# Place, Suite 330, Boston, MA 02111-1307 USA

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
include ../../common.mk

lib_LTLIBRARIES = libHuFS.la
SRC_DIR = src
HEADER_DIR = include
AM_CPPFLAGS += -I./include/ -I../Common/include
AM_CPPFLAGS += $(CLIENT_WAIT_FLAG) $(CLIENT_THREADS)

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)

libHuFS_la_SOURCES = CBB_basic.cpp	\
		     CBB_client.cpp 	\
		     CBB_stream.cpp 	\
		     CBB_posix.cpp 	\
		     CBB_stdio.cpp
# headers
libHuFS_la_SOURCES += CBB_basic.h	\
		      CBB_client.h	\
		      CBB_posix.h	\
		      CBB_stdio.h	\
		      CBB_stream.h

libHuFS_la_LDFLAGS = $(AM_LDFLAGS) -version-info 1:0:0
libHuFS_la_LIBADD = ../Common/libCommon.la

bin_PROGRAMS = hufs
hufs_SOURCES = src/CBB_fuse.cpp 

hufs_CPPFLAGS = $(AM_CPPFLAGS) -D_FILE_OFFSET_BITS=64
hufs_LDADD = libHuFS.la
hufs_LDFLAGS = $(AM_LDFLAGS) -ldl 
SUBDIRS = ../Common
