CBB_LTLIB= libCBB.so
CBB_SLIB= libCBB.a
MAIN = cbbfs

COMMON = ../Common/Client.o \
	 ../Common/CBB_rwlocker.o \
	 ../Common/CBB_communication_thread.o \
	 ../Common/CBB_cci.o\
	 ../Common/Comm_basic.o\
	 ../Common/CBB_IO_task.o\
	 ../Common/CBB_profiling.o

FUSE_SRC = CBB_fuse.cpp
BASE_SRC = CBB_client.cpp \
	   CBB_stream.cpp \
	   CBB_basic.cpp
LIB_SRC = CBB_posix.cpp \
	  CBB_stdio.cpp
SRC = $(FUSE_SRC) $(BASE_SRC) $(LIB_SRC)
FUSE_OBJ = $(FUSE_SRC:.cpp=.o)
BASE_OBJ = $(BASE_SRC:.cpp=.o)
LIB_OBJ = $(LIB_SRC:.cpp=.o)
OBJ = $(FUSE_OBJ) $(BASE_OBJ) $(LIB_OBJ)

DEP = $(SRC:.cpp=.d)

SRC_DIR = src
HEADER_DIR = include
INCLUDE_PATH = -I./include/ -I../Common/include
FUSE_HEADER = $(shell pkg-config --cflags fuse)
FUSE_LIB= $(shell pkg-config --libs fuse)

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)


$(CBB_SLIB):$(LIB_OBJ) $(BASE_OBJ) $(COMMON)
	ar -r $@ $^

$(CBB_LTLIB):$(LIB_OBJ) $(BASE_OBJ) $(COMMON)
	$(CC) -Wl,-soname=libCBB.so.0 $(LIB_FLAG) -o $@ $^ -ldl $(LIBRARIES)

$(MAIN):$(FUSE_OBJ) $(BASE_OBJ) $(COMMON) 
	$(CC) -o $@ $^ -ldl $(FUSE_LIB) $(LIBRARIES)

$(FUSE_OBJ):$(FUSE_SRC)
	$(CC) -c $(DEP_FLAG) $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) $(FUSE_HEADER) -o $@ $< 

%.o:%.cpp
	$(CC) -c $(CLIENT_WAIT_FLAG) $(DEP_FLAG) $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) -o $@ $< 

$(COMMON):
	cd ../Common && $(MAKE) $(notdir $@)

.PHONY:clean
clean:
	rm -f $(OBJ) $(DEP)
