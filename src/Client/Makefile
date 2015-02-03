CBB_LIB = libCBB.so

CBB = CBB.o
CBB_STREAM = CBB_stream.o
CBB_STDIO = CBB_stdio.o
CBB_POSIX = CBB_posix.o

OBJ = $(CBB) $(CBB_STREAM) $(CBB_STDIO) $(CBB_POSIX)

SRC_DIR = src
HEADER_DIR = include
CC = g++
INCLUDE_PATH = -I./include/ -I../Common/include

vpath %.cpp src/
vpath %.h include/

$(CBB_LIB):$(OBJ)
	$(CC) $(PRELOAD) $(FLAG) $(LIB_FLAG) -o $@ $^

$(CBB):$(SRC_DIR)/CBB.cpp $(HEADER_DIR)/CBB.h
	$(CC) -c $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) -o $@ $<

$(CBB_STREAM):$(SRC_DIR)/CBB_stream.cpp $(HEADER_DIR)/CBB_stream.h
	$(CC) -c $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) -o $@ $<

$(CBB_STDIO):$(SRC_DIR)/CBB_stdio.cpp $(HEADER_DIR)/CBB_stdio.h
	$(CC) -c $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) -o $@ $<

$(CBB_POSIX):$(SRC_DIR)/CBB_posix.cpp $(HEADER_DIR)/CBB_posix.h
	$(CC) -c $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) -o $@ $<

.PHONY:clean
clean:
	rm -f $(OBJ) $(CBB_LIB)
