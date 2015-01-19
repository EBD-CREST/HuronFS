CC = mpicc
LIB = ../../lib/

DEBUGER = gdb

OBJ = test3

NUMBER = 2

PRELOAD = LD_PRELOAD=$(LIB)/libBB.so

PRELOAD_LINK = -L$(LIB) -lBB
compile:$(OBJ)
	
$(OBJ):$(OBJ).c
	$(CC) -g -o $@ $< $(PRELOAD_LINK)

.PHONY:
clean:
	rm -f $(OBJ)

run:
	$(PRELOAD) mpirun -np $(NUMBER) -hostfile hostfile ./$(OBJ)

debug:
	$(PRELOAD) $(DEBUGER) ./$(OBJ)
