CBB_LIB= libCBB.so
MAIN = cbbfs

CLIENT = ../Common/Client.o

FUSE_SRC = CBB_fuse.cpp
LIB_SRC = CBB.cpp CBB_stream.cpp CBB_stdio.cpp CBB_posix.cpp
SRC = $(FUSE_SRC) $(LIB_SRC)
FUSE_OBJ = $(FUSE_SRC:.cpp=.o)
LIB_OBJ = $(LIB_SRC:.cpp=.o)
OBJ = $(FUSE_OBJ) $(LIB_OBJ)

DEP = $(SRC:.cpp=.d)

SRC_DIR = src
HEADER_DIR = include
INCLUDE_PATH = -I./include/ -I../Common/include
FUSE_HEADER = $(shell pkg-config --cflags fuse)
FUSE_LIB= $(shell pkg-config --libs fuse)

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)


$(CBB_LIB):$(OBJ) $(CLIENT)
	$(CC) $(LIB_FLAG) -o $@ $^ -ldl

$(MAIN):$(FUSE_OBJ) $(OBJ) $(CLIENT)
	$(CC) -o $@ $^ -ldl $(FUSE_LIB) 

$(FUSE_OBJ):$(FUSE_SRC)
	$(CC) -c $(DEP_FLAG) $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) $(FUSE_HEADER) -o $@ $< 

%.o:%.cpp
	$(CC) -c $(DEP_FLAG) $(PRELOAD) $(INCLUDE_PATH) $(LIB_FLAG) $(FLAG) -o $@ $< 

$(CLIENT):
	cd ../Common && $(MAKE) $(notdir $@)

.PHONY:clean
clean:
	rm -f $(OBJ) $(DEP)
