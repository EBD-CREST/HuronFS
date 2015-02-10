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

export LIB_FLAG=-shared -fPIC
export PRELOAD = -DCBB_PRELOAD
export FLAG=-g -Wall -DDEBUG
#export FLAG=-O3 -Wall

run:
	@echo 'run'
	@echo 'make Master'
	@echo 'or make IOnode'
	@echo 'or make Client'
	@echo 'or make all'

.PHONY:clean clean_all Master IOnode Client all
Master:
	$(MAKE) -C $(MASTER_DIR)
	mkdir -p bin lib
	mv $(MASTER_LIB) lib
	mv $(MASTER) bin
	echo $$ld_library_path|grep `pwd` || export ld_library_path=`pwd`/lib:$$ld_library_path

IOnode:
	$(MAKE) -C $(IONODE_DIR)
	mkdir -p bin lib
	mv $(IONODE_LIB) lib
	mv $(IONODE) bin
	echo $$LD_LIBRARY_PATH|grep `pwd` || export LD_LIBRARY_PATH=`pwd`/lib:$$LD_LIBRARY_PATH

Client:
	$(MAKE) -C $(CLIENT_DIR)
	mkdir -p bin lib
	mv $(CLIENT_LIB) lib
	echo $$LD_LIBRARY_PATH|grep `pwd` || export LD_LIBRARY_PATH=`pwd`/lib:$$LD_LIBRARY_PATH

all:Master IOnode Client

clean:
	rm -f $(BIN)/*
	rm -f $(LIB)/*
	$(MAKE) -C $(MASTER_DIR) clean
	$(MAKE) -C $(IONODE_DIR) clean
	$(MAKE) -C $(CLIENT_DIR) clean
	$(MAKE) -C $(COMMON_DIR) clean

clean_all:clean
	$(MAKE) -C tests clean
