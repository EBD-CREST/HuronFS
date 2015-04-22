MAIN = IOnode
SRC_DIR = src
HEADER_DIR = include
INCLUDE_PATH = -I./include -I../Common/include
SERVER = ../Common/Server.o
CLIENT = ../Common/Client.o
LINK_PATH = -L./

SRC = IOnode.cpp node_main.cpp
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)

$(MAIN):$(OBJ) $(SERVER) $(CLIENT)
	$(CC) -o $@ $^

%.o:%.cpp
	$(CC) -c $(DEP_FLAG) $(FLAG) $(LIB_FLAG) $(INCLUDE_PATH) -o $@ $<

$(SERVER) $(CLIENT):
	cd ../Common && $(MAKE) $(notdir $@)

.PHONY:clean
clean:
	rm -f $(OBJ) $(DEP) $(MAIN)
