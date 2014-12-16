CC = gcc

OBJ = test1

LINK_FLAG = -L../../lib -lBB -ldl
compile:$(OBJ)
	
$(OBJ):$(OBJ).c
	$(CC) -DBB_PRELOAD -o $@ $< $(LINK_FLAG)

.PHONY:
clean:
	rm -f $(OBJ)
