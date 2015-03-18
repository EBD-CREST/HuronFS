LIB = libMaster.so
OBJ = Master 
SRC_DIR = src
HEADER_DIR = include
INCLUDE_PATH = -I./include -I../Common/include
LINK_PATH = -L./
SERVER = ../Common/Server.o

vpath %.cpp src/
vpath %.h include/

$(OBJ):$(SRC_DIR)/master_main.cpp $(LIB)
	$(CC) $(FLAG) $(INCLUDE_PATH) $(LINK_PATH) -o $@ $< -lMaster

$(LIB):$(SRC_DIR)/Master.cpp $(HEADER_DIR)/Master.h $(SERVER)
	$(CC) $(FLAG) $(LIB_FLAG) $(INCLUDE_PATH) -o $@ $^

$(SERVER):
	cd ../Common && $(MAKE) $(notdir $@)

.PHONY:clean
clean:
	rm -f $(LIB) $(OBJ)
