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
noinst_LIBRARIES= libCommon.a
SRC_DIR = src
HEADER_DIR = include
AM_CPPFLAGS += -I./include/

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)

libCommon_a_SOURCES = Server.cpp \
		      Client.cpp \
		      CBB_mutex_lock.cpp \
		      CBB_rwlock.cpp \
		      CBB_communication_thread.cpp \
		      CBB_request_handler.cpp \
		      CBB_serialization.cpp\
		      CBB_remote_task.cpp \
		      CBB_heart_beat.cpp \
		      CBB_data_sync.cpp \
		      CBB_profiling.cpp \
		      Comm_basic.cpp \
		      CBB_memory_pool.cpp \
		      CBB_IO_task.cpp 


if USING_CCI
libCommon_a_SOURCES += CBB_cci.cpp
else
libCommon_a_SOURCES += CBB_tcp.cpp
endif
