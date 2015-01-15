CC = gcc

OBJ = test3

LINK_FLAG = -L../../lib -lBB -ldl
compile:$(OBJ)
	
$(OBJ):$(OBJ).c
	$(CC) -g -o $@ $< $(LINK_FLAG)

.PHONY:
clean:
	rm -f $(OBJ)
