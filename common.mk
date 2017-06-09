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

AM_CPPFLAGS = -DCBB_PRELOAD -DLOG
AM_CXXFLAGS = -Wall -std=c++11
AM_LDFLAGS = -lpthread 

SYNC_FLAG = -DSTRICT_SYNC_DATA -DSYNC_DATA_WITH_REPLY
SERVER_WAIT_FLAG = -DBUSY_WAIT -DSLEEP_YIELD
CLIENT_WAIT_FLAG = -DBUSY_WAIT -DSLEEP_YIELD

SWAP_ALGORITHM = -DLRU

if WRITE_BACK
WRITE_BACK = -DWRITE_BACK
if ASYNC_WRITE_BACK
WRITE_BACK += -DASYNC
endif
endif


if DEBUG
AM_CPPFLAGS += -DDEBUG
endif

if USING_CCI
AM_CPPFLAGS += -DCCI $(CCI_INCLUDE)
AM_LDFLAGS +=  ${CCI_LIB} -lcci
else
AM_CPPFLAGS += -DTCP
endif

