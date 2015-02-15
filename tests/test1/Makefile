CC = gcc
CBB = ../../lib/libCBB.so

OBJ = test1

DEBUGER = gdb

PRELOAD = LD_PRELOAD=$(CBB)

compile:$(OBJ)
	
$(OBJ):$(OBJ).c
	$(CC) -o $@ $< 

.PHONY:clean
clean:
	rm -f $(OBJ)

run:
	$(PRELOAD) ./$(OBJ)

debug:
	$(PRELOAD) $(DEBUGER) ./$(OBJ)
