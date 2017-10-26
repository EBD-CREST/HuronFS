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
AM_CPPFLAGS += -I./include/ -I../Common/include
SRC_DIR = src
HEADER_DIR = include
AM_CPPFLAGS += $(SERVER_WAIT_FLAG) $(WRITE_BACK) $(SYNC_FLAG) $(SWAP_ALGORITHM)

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)

bin_PROGRAMS = IOnode

IOnode_SOURCES = IOnode.cpp	\
		 node_main.cpp	\
		 IOnode_basic.cpp

#headers
IOnode_SOURCES += IOnode.h	\
		  IOnode_basic.h

IOnode_LDADD = ../Common/libCommon.la
