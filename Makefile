MASTER_LIB = Master
IONODE_LIB = IOnode
MASTER = master_main
IONODE = node_main
INCLUDE = include
SRC = src
LIB = lib
BIN = bin

CC = g++
#FLAG = -O3 -Wall
FLAG = -g -Wall
run:
	@echo 'run make master'
	@echo 'or make ionode'
master:
	$(CC) -c $(FLAG) -I . -o $(LIB)/$(MASTER_LIB).o $(SRC)/$(MASTER_LIB).cpp
	$(CC) $(FLAG) -I . -o $(BIN)/$(MASTER) $(LIB)/$(MASTER_LIB).o $(SRC)/$(MASTER).cpp

ionode:
	$(CC) -c $(FLAG) -I . -o $(LIB)/$(IONODE_LIB).o $(SRC)/$(IONODE_LIB).cpp
	$(CC) $(FLAG) -I . -o $(BIN)/$(IONODE) $(LIB)/$(IONODE_LIB).o $(SRC)/$(IONODE).cpp
clean:
	rm $(BIN)/*
	rm $(LIB)/*
