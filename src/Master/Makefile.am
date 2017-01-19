# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
#
include ../../common.mk
AM_CPPFLAGS += -I./include/ -I../Common/include
SRC_DIR = src
HEADER_DIR = include
AM_CPPFLAGS += $(SERVER_WAIT_FLAG)

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)

bin_PROGRAMS = Master

Master_SOURCES = Master.cpp	 \
		 master_main.cpp \
		 Master_basic.cpp

#headers
Master_SOURCES += Master.h	\
		  Master_basic.h

Master_LDADD = ../Common/libCommon.a
