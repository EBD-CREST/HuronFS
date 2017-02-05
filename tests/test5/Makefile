CC = gcc 
LIB = ../../lib/

DEBUGER = gdb

OBJ = test5

NUMBER = 2

compile:$(OBJ)
	
$(OBJ):$(OBJ).c
	$(CC) -g -o $@ $< $(PRELOAD_LINK)

.PHONY:
clean:
	rm -f $(OBJ)

run:
	$(PRELOAD)  ./$(OBJ)

debug:
	$(PRELOAD) $(DEBUGER) ./$(OBJ)
