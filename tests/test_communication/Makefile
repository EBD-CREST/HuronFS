CC = g++
SRC = client.cpp server.cpp
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)
SRC_DIR = src
HEADER_DIR = include
INCLUDE_PATH = -I../include

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)

FLAG = -g -std=c++0x

%.o:%.cpp
	$(CC) $(FLAG) -c $(DEP_FLAG) $(LIB_FLAG) $(INCLUDE_PATH) -o $@ $<

server:server.o ../Server.o ../CBB_task_parallel.o ../CBB_request_handler.o ../CBB_communication_thread.o
	$(CC) $(FLAG) -o $@ $^ -lpthread

client:client.o
	$(CC) $(FLAG) -o $@ $<

.PHONY:clean
clean:
	rm -f $(OBJ) $(DEP)
