LIB = lib
BIN = bin
SRC = src
MASTER_DIR = $(SRC)/Master
IONODE_DIR = $(SRC)/IOnode
CLIENT_DIR = $(SRC)/Client
COMMON_DIR = $(SRC)/Common

MASTER_LIB = $(MASTER_DIR)/libMaster.so
MASTER = $(MASTER_DIR)/Master

IONODE_LIB = $(IONODE_DIR)/libIOnode.so
IONODE = $(IONODE_DIR)/IOnode

CLIENT_LIB = $(CLIENT_DIR)/libCBB.so
CLIENT_FUSE= $(CLIENT_DIR)/CBB_fuse

export CC=g++
export LIB_FLAG=-shared -fPIC
export PRELOAD = -DCBB_PRELOAD
export FLAG=-O0 -g -Wall -DDEBUG
#export FLAG=-O3 -Wall 

run:
	@echo 'run'
	@echo 'make Master'
	@echo 'or make IOnode'
	@echo 'or make Client'
	@echo 'or make Client_fuse'
	@echo 'or make all'

.PHONY:clean clean_all Master IOnode Client all
Master:
	$(MAKE) -C $(MASTER_DIR)
	mkdir -p bin lib
	mv $(MASTER_LIB) lib
	mv $(MASTER) bin

IOnode:
	$(MAKE) -C $(IONODE_DIR)
	mkdir -p bin lib
	mv $(IONODE_LIB) lib
	mv $(IONODE) bin

Client:
	$(MAKE) -C $(CLIENT_DIR)
	mkdir -p lib
	mv $(CLIENT_LIB) lib

Client_fuse:
	$(MAKE) -C $(CLIENT_DIR) CBB_fuse
	mkdir -p bin 
	mv $(CLIENT_FUSE) bin

all:Master IOnode Client Client_fuse

clean:
	rm -f $(BIN)/*
	rm -f $(LIB)/*
	$(MAKE) -C $(MASTER_DIR) clean
	$(MAKE) -C $(IONODE_DIR) clean
	$(MAKE) -C $(CLIENT_DIR) clean
	$(MAKE) -C $(COMMON_DIR) clean

clean_all:clean
	$(MAKE) -C tests clean
