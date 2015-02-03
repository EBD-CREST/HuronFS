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
	@echo 'run make Master'
	@echo 'or make IOnode'
	@echo 'or make User_main'
	@echo 'or make Client'
	@echo 'or make all'

.PHONY:clean clean_all
Master:
	cd $(MASTER_DIR) && $(MAKE)
	mkdir -p bin lib
	mv $(MASTER_LIB) lib
	mv $(MASTER) bin
	echo $$ld_library_path|grep `pwd` || export ld_library_path=`pwd`/lib:$$ld_library_path

IOnode:
	cd $(IONODE_DIR) && $(MAKE)
	mkdir -p bin lib
	mv $(IONODE_LIB) lib
	mv $(IONODE) bin
	echo $$LD_LIBRARY_PATH|grep `pwd` || export LD_LIBRARY_PATH=`pwd`/lib:$$LD_LIBRARY_PATH

Client:
	cd $(CLIENT_DIR) && $(MAKE)
	mkdir -p bin lib
	mv $(CLIENT_LIB) lib
	echo $$LD_LIBRARY_PATH|grep `pwd` || export LD_LIBRARY_PATH=`pwd`/lib:$$LD_LIBRARY_PATH

all: 
	mkdir -p bin lib
	mv *.so lib
	mv $(MASTER_OBJ) bin
	mv $(IONODE_OBJ) bin
	rm -f *.o
	echo $$LD_LIBRARY_PATH|grep `pwd` || export LD_LIBRARY_PATH=`pwd`/lib:$$LD_LIBRARY_PATH

clean:
	rm -f $(BIN)/*
	rm -f $(LIB)/*
	cd $(MASTER_DIR) && make clean
	cd $(IONODE_DIR) && make clean
	cd $(CLIENT_DIR) && make clean
	cd $(COMMON_DIR) && make clean

clean_all:clean
	cd tests && $(MAKE) clean
