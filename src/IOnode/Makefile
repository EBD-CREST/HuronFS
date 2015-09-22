MAIN = IOnode
SRC_DIR = src
HEADER_DIR = include
INCLUDE_PATH = -I./include -I../Common/include
COMMON = ../Common/Server.o ../Common/CBB_connector.o ../Common/CBB_task_parallel.o ../Common/CBB_communication_thread.o ../Common/CBB_request_handler.o ../Common/CBB_rwlocker.o
//CLIENT = ../Common/Client.o
LINK_PATH = -L./

SRC = IOnode.cpp node_main.cpp
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)

$(MAIN):$(OBJ) $(COMMON)
	$(CC) -o $@ $^ -lpthread

%.o:%.cpp
	$(CC) -c $(DEP_FLAG) $(FLAG) $(INCLUDE_PATH) -o $@ $<

$(COMMON):
	cd ../Common && $(MAKE) $(notdir $@)

.PHONY:clean
clean:
	rm -f $(OBJ) $(DEP) $(MAIN)
