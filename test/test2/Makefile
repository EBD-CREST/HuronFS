CC = gcc

OBJ = test2

LINK_FLAG = -L../../lib -lBB -ldl
compile:$(OBJ)
	
$(OBJ):$(OBJ).c
	$(CC) -g -DBB_PRELOAD -o $@ $< $(LINK_FLAG)

.PHONY:
clean:
	rm -f $(OBJ)
