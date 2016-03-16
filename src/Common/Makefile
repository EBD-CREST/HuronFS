SRC = Server.cpp Client.cpp CBB_socket.cpp Thread_pool.cpp CBB_mutex_locker.cpp CBB_rwlocker.cpp CBB_dir.cpp CBB_communication_thread.cpp CBB_request_handler.cpp CBB_connector.cpp CBB_remote_task.cpp CBB_heart_beat.cpp CBB_data_sync.cpp CBB_serialization.cpp
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)
SRC_DIR = src
HEADER_DIR = include
INCLUDE_PATH = -I./include

vpath %.cpp $(SRC_DIR)
vpath %.h $(HEADER_DIR)

%.o:%.cpp
	$(CC) $(FLAG) -c $(DEP_FLAG) $(LIB_FLAG) $(INCLUDE_PATH) -o $@ $<

.PHONY:clean
clean:
	rm -f $(OBJ) $(DEP)
