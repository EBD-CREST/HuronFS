MASTER_LIB = Master
IONODE_LIB = IOnode
SERVER_LIB = Server
CLIENT_LIB = Client
MASTER = master_main
IONODE = node_main
INCLUDE = include
SRC = src
LIB = lib
BIN = bin

CC = g++
FLAG = -O3 -Wall
#FLAG = -g -Wall
run:
	@echo 'run make Master'
	@echo 'or make IOnode'
$(SERVER_LIB).o:$(SRC)/$(SERVER_LIB).cpp $(INCLUDE)/$(SERVER_LIB).h
	$(CC) -c $(FLAG) -I . -o $(SERVER_LIB).o $(SRC)/$(SERVER_LIB).cpp

$(CLIENT_LIB).o:$(SRC)/$(CLIENT_LIB).cpp  $(INCLUDE)/$(CLIENT_LIB).h
	$(CC) -c $(FLAG) -I . -o $(CLIENT_LIB).o $(SRC)/$(CLIENT_LIB).cpp

$(MASTER).o:$(SRC)/$(MASTER_LIB).cpp $(INCLUDE)/$(MASTER_LIB).h
	$(CC) -c $(FLAG) -I . -o $(MASTER_LIB).o $(SRC)/$(MASTER_LIB).cpp

$(MASTER):$(SERVER_LIB).o $(MASTER).o $(SRC)/$(MASTER).cpp
	$(CC) $(FLAG) -I . -o $(MASTER) $(SERVER_LIB).o $(MASTER_LIB).o $(SRC)/$(MASTER).cpp

$(IONODE).o:$(SRC)/$(IONODE_LIB).cpp $(INCLUDE)/$(IONODE_LIB).h
	$(CC) -c $(FLAG) -I . -o $(IONODE_LIB).o $(SRC)/$(IONODE_LIB).cpp

$(IONODE):$(SERVER_LIB).o $(IONODE).o $(SRC)/$(IONODE).cpp $(CLIENT_LIB).o
	$(CC) $(FLAG) -I . -o $(IONODE) $(SERVER_LIB).o $(CLIENT_LIB).o $(IONODE_LIB).o $(SRC)/$(IONODE).cpp

.PHONY:
Master:$(MASTER)
	mkdir -p lib
	mkdir -p bin
	rm -f bin/$(MASTER)
	rm -f lib/$(MASTER_LIB).o lib/$(SERVER_LIB).o
	mv $(MASTER) bin
	mv $(MASTER_LIB).o lib
	mv $(SERVER_LIB).o lib

IOnode:$(IONODE)
	mkdir -p lib
	mkdir -p bin
	rm -f bin/$(IONODE)
	rm -f lib/$(IONODE_LIB).o lib/$(IONODE_LIB).o
	mv $(IONODE) bin
	mv $(IONODE_LIB).o lib
	mv $(SERVER_LIB).o lib

clean:
	rm $(BIN)/*
	rm $(LIB)/*
