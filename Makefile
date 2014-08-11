MASTER = Master
IONODE = IOnode
CC = g++
FLAG = -O2

master:
	$(CC) $(FLAG) -o $(MASTER) $(MASTER).cpp
