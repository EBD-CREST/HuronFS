# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
include ../../common.mk
AM_CPPFLAGS += -I./include/ -I../Common/include
SRC_DIR = src
HEADER_DIR = include
AM_CPPFLAGS += $(SERVER_WAIT_FLAG) $(WRITE_BACK) $(SYNC_FLAG)

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)

bin_PROGRAMS = IOnode

IOnode_SOURCES = IOnode.cpp	\
		 node_main.cpp

#headers
IOnode_SOURCES += IOnode.h

IOnode_LDADD = ../Common/libCommon.a
