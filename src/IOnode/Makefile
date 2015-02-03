LIB = libIOnode.so
OBJ = IOnode
SRC_DIR = src
HEADER_DIR = include
CC = g++
INCLUDE_PATH = -I./include -I../Common/include
SERVER = ../Common/Server.o
CLIENT = ../Common/Client.o
LINK_PATH = -L./

vpath %.cpp src/
vpath %.h include/

$(OBJ):$(SRC_DIR)/node_main.cpp $(LIB)
	$(CC) $(FLAG) $(INCLUDE_PATH) $(LINK_PATH) -o $@ $< -lIOnode

$(LIB):$(SRC_DIR)/IOnode.cpp $(HEADER_DIR)/IOnode.h $(SERVER) $(CLIENT)
	$(CC) $(FLAG) $(LIB_FLAG) $(INCLUDE_PATH) -o $@ $^

$(SERVER) $(CLIENT):
	cd ../Common && $(MAKE) $(notdir $@)

.PHONY:clean
clean:
	rm -f $(LIB) $(OBJ)
