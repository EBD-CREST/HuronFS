MASTER_LIB = Master
IONODE_LIB = IOnode
SERVER_LIB = Server
CLIENT_LIB = Client
QUERY_LIB = Query_Client
USER_LIB = User_Client

MASTER = master_main
IONODE = node_main
USER_MAIN = user_main
USER_CLIENT = user_client

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
	@echo 'or make User_main'
	@echo 'or make User_client'

$(SERVER_LIB).o:$(SRC)/$(SERVER_LIB).cpp $(INCLUDE)/$(SERVER_LIB).h
	$(CC) -c $(FLAG) -I . -o $(SERVER_LIB).o $(SRC)/$(SERVER_LIB).cpp

$(CLIENT_LIB).o:$(SRC)/$(CLIENT_LIB).cpp  $(INCLUDE)/$(CLIENT_LIB).h
	$(CC) -c $(FLAG) -I . -o $(CLIENT_LIB).o $(SRC)/$(CLIENT_LIB).cpp

$(QUERY_LIB).o:$(SRC)/$(QUERY_LIB).cpp $(INCLUDE)/$(QUERY_LIB).h
	$(CC) -c $(FLAG) -I . -o $(QUERY_LIB).o $(SRC)/$(QUERY_LIB).cpp

$(USER_LIB).o:$(SRC)/$(USER_LIB).cpp $(INCLUDE)/$(USER_LIB).h
	$(CC) -c $(FLAG) -I . -lrt -o $(USER_LIB).o $(SRC)/$(USER_LIB).cpp

$(MASTER).o:$(SRC)/$(MASTER_LIB).cpp $(INCLUDE)/$(MASTER_LIB).h
	$(CC) -c $(FLAG) -I . -o $(MASTER_LIB).o $(SRC)/$(MASTER_LIB).cpp

$(IONODE).o:$(SRC)/$(IONODE_LIB).cpp $(INCLUDE)/$(IONODE_LIB).h
	$(CC) -c $(FLAG) -I . -o $(IONODE_LIB).o $(SRC)/$(IONODE_LIB).cpp

$(MASTER):$(SERVER_LIB).o $(MASTER).o $(SRC)/$(MASTER).cpp
	$(CC) $(FLAG) -I . -o $(MASTER) $(SERVER_LIB).o $(MASTER_LIB).o $(SRC)/$(MASTER).cpp

$(IONODE):$(SERVER_LIB).o $(IONODE).o $(SRC)/$(IONODE).cpp $(CLIENT_LIB).o
	$(CC) $(FLAG) -I . -o $(IONODE) $(SERVER_LIB).o $(CLIENT_LIB).o $(IONODE_LIB).o $(SRC)/$(IONODE).cpp

$(USER_MAIN):$(CLIENT_LIB).o $(QUERY_LIB).o $(SRC)/$(USER_MAIN).cpp
	$(CC) $(FLAG) -I . -o $(USER_MAIN) $(CLIENT_LIB).o $(QUERY_LIB).o $(SRC)/$(USER_MAIN).cpp

$(USER_CLIENT):$(CLIENT_LIB).o $(USER_LIB).o $(SRC)/$(USER_CLIENT).cpp
	$(CC) $(FLAG) -I . -lrt -o $(USER_CLIENT) $(USER_LIB).o $(CLIENT_LIB).o $(SRC)/$(USER_CLIENT).cpp

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
	rm -f lib/$(IONODE_LIB).o lib/$(SERVER_LIB).o lib/$(CLIENT_LIB).o
	mv $(IONODE) bin
	mv $(IONODE_LIB).o lib
	mv $(SERVER_LIB).o lib
	mv $(CLIENT_LIB).o lib

User_main:$(USER_MAIN)
	mkdir -p lib
	mkdir -p bin
	rm -f bin/$(USER_MAIN)
	rm -f lib/$(QUERY_LIB).o lib/$(CLIENT_LIB).o
	mv $(CLIENT_LIB).o lib
	mv $(QUERY_LIB).o lib
	mv $(USER_MAIN) bin

User_client:$(USER_CLIENT)
	mkdir -p lib
	mkdir -p bin
	rm -f bin/$(USER_CLIENT)
	rm -f lib/$(USER_LIB).o lib/$(CLIENT_LIB).o
	mv $(CLIENT_LIB).o lib
	mv $(USER_LIB).o lib
	mv $(USER_CLIENT) bin

clean:
	rm $(BIN)/*
	rm $(LIB)/*
