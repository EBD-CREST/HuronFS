LIB = lib
BIN = bin
SRC = src
MASTER_DIR = $(SRC)/Master
IONODE_DIR = $(SRC)/IOnode
CLIENT_DIR = $(SRC)/Client
COMMON_DIR = $(SRC)/Common

MASTER = $(MASTER_DIR)/Master

IONODE = $(IONODE_DIR)/IOnode

CLIENT_SLIB = $(CLIENT_DIR)/libCBB.a
CLIENT_LTLIB = $(CLIENT_DIR)/libCBB.so
CLIENT_FUSE= $(CLIENT_DIR)/cbbfs

export CC=g++
export LIB_FLAG=-shared -fPIC
export PRELOAD = -DCBB_PRELOAD
export DEP_FLAG = -MMD -MP
export FLAG=-O3 -g -Wall -std=c++0x

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
	mkdir -p bin
	mv $(MASTER) bin

IOnode:
	$(MAKE) -C $(IONODE_DIR)
	mkdir -p bin
	mv $(IONODE) bin

Client:
	$(MAKE) -C $(CLIENT_DIR) libCBB.a libCBB.so
	mkdir -p lib
	mv $(CLIENT_LTLIB) lib
	mv $(CLIENT_SLIB) lib

Client_fuse:
	$(MAKE) -C $(CLIENT_DIR) cbbfs
	mkdir -p bin 
	mv $(CLIENT_FUSE) bin

Master_gdb:
	export FLAG_GDB=-O0 -g -Wall -std=c++0x -DDEBUG
	$(MAKE) -C $(MASTER_DIR)
	mkdir -p bin
	mv $(MASTER) bin

IOnode_gdb:
	export FLAG_GDB=-O0 -g -Wall -std=c++0x -DDEBUG
	$(MAKE) -C $(IONODE_DIR)
	mkdir -p bin
	mv $(IONODE) bin

Client_gdb:
	export FLAG_GDB=-O0 -g -Wall -std=c++0x -DDEBUG
	$(MAKE) -C $(CLIENT_DIR) libCBB.a libCBB.so
	mkdir -p lib
	mv $(CLIENT_LTLIB) lib
	mv $(CLIENT_SLIB) lib

Client_fuse_gdb:
	export FLAG_GDB=-O0 -g -Wall -std=c++0x -DDEBUG
	$(MAKE) -C $(CLIENT_DIR)
	mkdir -p bin 
	mv $(CLIENT_FUSE) bin

all:Master IOnode Client_fuse Client

all_gdb:Master_gdb IOnode_gdb Client_fuse_gdb Client_gdb

clean:
	rm -f $(BIN)/*
	rm -f $(LIB)/*
	$(MAKE) -C $(MASTER_DIR) clean
	$(MAKE) -C $(IONODE_DIR) clean
	$(MAKE) -C $(CLIENT_DIR) clean
	$(MAKE) -C $(COMMON_DIR) clean

clean_all:clean
	$(MAKE) -C tests clean
