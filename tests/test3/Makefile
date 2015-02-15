CC = mpicc
LIB = ../../lib/

DEBUGER = gdb

OBJ = test3

NUMBER = 2

PRELOAD = LD_PRELOAD=$(LIB)/libCBB.so

#PRELOAD_LINK = -L$(LIB) -lCBB
compile:$(OBJ)
	
$(OBJ):$(OBJ).c
	$(CC) -g -o $@ $< $(PRELOAD_LINK)

.PHONY:
clean:
	rm -f $(OBJ)

run:
	$(PRELOAD) ./$(OBJ)

debug:
	$(PRELOAD) $(DEBUGER) ./$(OBJ)
