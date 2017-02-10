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

CC = g++
SRC = client.cpp server.cpp
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)
SRC_DIR = src
HEADER_DIR = include
INCLUDE_PATH = -I../include

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)

FLAG = -g -std=c++0x

%.o:%.cpp
	$(CC) $(FLAG) -c $(DEP_FLAG) $(LIB_FLAG) $(INCLUDE_PATH) -o $@ $<

server:server.o ../Server.o ../CBB_task_parallel.o ../CBB_request_handler.o ../CBB_communication_thread.o
	$(CC) $(FLAG) -o $@ $^ -lpthread

client:client.o
	$(CC) $(FLAG) -o $@ $<

.PHONY:clean
clean:
	rm -f $(OBJ) $(DEP)
