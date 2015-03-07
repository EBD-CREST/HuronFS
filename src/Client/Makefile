CBB_LIB = libCBB.so

CBB = CBB.o
CBB_STREAM = CBB_stream.o
CBB_STDIO = CBB_stdio.o
CBB_POSIX = CBB_posix.o
CLIENT = ../Common/Client.o
CLIENT_FUSE = CBB_fuse

OBJ = $(CBB) $(CBB_STREAM) $(CBB_STDIO) $(CBB_POSIX) 

SRC_DIR = src
HEADER_DIR = include
CC = g++
INCLUDE_PATH = -I./include/ -I../Common/include
FUSE_HEADER = $(shell pkg-config --cflags --libs fuse)

vpath %.cpp src/
vpath %.h include/


$(CBB_LIB):$(OBJ) $(CLIENT)
	$(CC) $(PRELOAD) $(FLAG) $(LIB_FLAG) -o $@ $^ -ldl

$(CBB_STDIO):$(SRC_DIR)/CBB_stdio.cpp $(CBB_POSIX) $(HEADER_DIR)/CBB_stdio.h
	$(CC) -c $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) -o $@ $< 

$(CBB_POSIX):$(SRC_DIR)/CBB_posix.cpp $(CBB_STREAM) $(HEADER_DIR)/CBB_posix.h
	$(CC) -c $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) -o $@ $<

$(CBB_STREAM):$(SRC_DIR)/CBB_stream.cpp $(CBB) $(HEADER_DIR)/CBB_stream.h
	$(CC) -c $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) -o $@ $<

$(CBB):$(SRC_DIR)/CBB.cpp $(HEADER_DIR)/CBB.h
	$(CC) -c $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) -o $@ $<

$(CLIENT):
	cd ../Common && $(MAKE) $(notdir $@)

$(CLIENT_FUSE):$(SRC_DIR)/CBB_fuse.cpp $(CBB_STREAM) $(CLIENT)
	$(CC) $(INCLUDE_PATH) $(FLAG) $(FUSE_HEADER) -o $@ $^ $(CBB)


.PHONY:clean
clean:
	rm -f $(OBJ) $(CBB_LIB) $(CLIENT_FUSE)
