SERVER = Server.o
CLIENT = Client.o

SRC_DIR = src
HEADER_DIR = include
CC = g++
INCLUDE_PATH = -I./

vpath %.cpp src/
vpath %.h include/

$(SERVER):$(SRC_DIR)/Server.cpp $(HEADER_DIR)/Server.h
	$(CC) $(FLAG) -c $(LIB_FLAG) $(INCLUDE_PATH) -o $@ $<

$(CLIENT):$(SRC_DIR)/Client.cpp $(HEADER_DIR)/Client.h
	$(CC) $(FLAG) -c $(LIB_FLAG) $(INCLUDE_PATH) -o $@ $<

.PHONY:clean
clean:
	rm -f $(CLIENT) $(SERVER)
