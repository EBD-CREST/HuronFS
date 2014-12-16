MASTER_LIB = Master
IONODE_LIB = IOnode
SERVER_LIB = Server
CLIENT_LIB = Client
QUERY_LIB = Query_Client
POSIX_LIB = BB_posix
BB_LIB = BB

MASTER = master_main
IONODE = node_main
USER_MAIN = user_main
USER_CLIENT = user_client
USER_CLIENT = user_client

INCLUDE = include
SRC = src
LIB = lib
BIN = bin

CC = g++
FLAG = -O3 -Wall
#FLAG = -g -Wall

LIBFLAG = -shared -fPIC

INCLUDE_FLAG = -I./

LINK_FLAG = -L./

run:
	@echo 'run make Master'
	@echo 'or make IOnode'
	@echo 'or make User_main'
	@echo 'or make User_client'

Server.o:$(SRC)/$(SERVER_LIB).cpp $(INCLUDE)/$(SERVER_LIB).h
	$(CC) -c $(FLAG) -fPIC $(INCLUDE_FLAG) -o $@ $<

Client.o:$(SRC)/$(CLIENT_LIB).cpp $(INCLUDE)/$(CLIENT_LIB).h
	$(CC) -c $(FLAG) -fPIC $(INCLUDE_FLAG) -o $@ $<

#$(QUERY_LIB).o:$(SRC)/$(QUERY_LIB).cpp $(INCLUDE)/$(QUERY_LIB).h
#	$(CC) -c $(FLAG) $(INCLUDE_FLAG) -o $(QUERY_LIB).o $(SRC)/$(QUERY_LIB).cpp

#libuser.so:$(SRC)/$(USER_LIB).cpp $(INCLUDE)/$(USER_LIB).h Client.o
#	$(CC) $(FLAG) $(LIBFLAG) $(INCLUDE_FLAG) -o $@ $< Client.o

BB.o:$(SRC)/$(BB_LIB).cpp $(INCLUDE)/$(BB_LIB).h 
	$(CC) $(FLAG) -c -fPIC $(INCLUDE_FLAG) -o $@ $<
	
libBB.so:$(SRC)/$(POSIX_LIB).cpp Client.o BB.o
	$(CC) -DBB_PRELOAD $(FLAG) $(LIBFLAG) $(INCLUDE_FLAG) -o $@ $< Client.o BB.o

libmaster.so:$(SRC)/$(MASTER_LIB).cpp $(INCLUDE)/$(MASTER_LIB).h Server.o
	$(CC) $(FLAG) $(LIBFLAG) $(INCLUDE_FLAG) -o $@ $< Server.o

libionode.so:$(SRC)/$(IONODE_LIB).cpp $(INCLUDE)/$(IONODE_LIB).h $(Server.o) Client.o Server.o
	$(CC) $(FLAG) $(LIBFLAG) $(INCLUDE_FLAG) -o $@ $< Server.o Client.o

$(MASTER):$(SRC)/$(MASTER).cpp libmaster.so 
	$(CC) $(FLAG) $(LINK_FLAG) $(INCLUDE_FLAG) -o $@ $< -lmaster

$(IONODE):$(SRC)/$(IONODE).cpp libionode.so 
	$(CC) $(FLAG) $(LINK_FLAG) $(INCLUDE_FLAG) -o $@ $< -lionode

#$(USER_MAIN):lib $(QUERY_LIB).o $(SRC)/$(USER_MAIN).cpp
#	$(CC) $(FLAG) $(INCLUDE_FLAG) -o $(USER_MAIN) $(CLIENT_LIB).o $(QUERY_LIB).o $(SRC)/$(USER_MAIN).cpp

$(USER_CLIENT):$(SRC)/$(USER_CLIENT).cpp libBB.so 
	$(CC) $(FLAG) $(LINK_FLAG) $(INCLUDE_FLAG) -o $@ $< -luser -lrt

.PHONY:
Master:$(MASTER)
	mkdir -p bin lib
	mv $< bin
	mv *.so lib
	rm -f *.o

IOnode:$(IONODE)
	mkdir -p bin lib
	mv $< bin
	mv *.so lib
	rm -f *.o

User_main:$(USER_MAIN)
	mkdir -p bin lib
	mv $< bin
	mv *.so lib
	rm -f *.o

client.so:libBB.so
	mkdir -p bin lib
	mv $< lib
	rm -f *.o

clean:
	rm $(BIN)/*
	rm $(LIB)/*
