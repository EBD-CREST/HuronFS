CC = gcc
BB = ../../lib/libBB.so

OBJ = test1

DEBUGER = gdb

PRELOAD = LD_PRELOAD=$(BB)

compile:$(OBJ)
	
$(OBJ):$(OBJ).c
	$(CC) -o $@ $< 

.PHONY:
clean:
	rm -f $(OBJ)

run:
	$(PRELOAD) ./$(OBJ)

debug:
	$(PRELOAD) $(DEBUGER) ./$(OBJ)
