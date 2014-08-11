MASTER = Master
IONODE = IOnode
CC = gcc
FLAG = -O2

master:
	$(CC) $(FLAG) -o $(MASTER) $(MASTER).cpp
