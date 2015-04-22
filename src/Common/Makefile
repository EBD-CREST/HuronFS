SRC = Server.cpp Client.cpp
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
