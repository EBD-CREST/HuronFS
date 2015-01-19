CC = gcc
LIB = ../../lib/

DEBUGER = gdb

OBJ = test2

PRELOAD = LD_PRELOAD=$(LIB)/libBB.so

PRELOAD_LINK = -L$(LIB) -lBB
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
