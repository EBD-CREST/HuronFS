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
		      CBB_IO_task.cpp 


if USING_CCI
libCommon_a_SOURCES += CBB_cci.cpp
else
libCommon_a_SOURCES += CBB_tcp.cpp
endif
