MASTER = Master
IONODE = IOnode
CC = g++
FLAG = -O3

master:
	$(CC) $(FLAG) -c -o $(MASTER) $(MASTER).cpp
