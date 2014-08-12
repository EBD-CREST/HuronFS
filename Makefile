MASTER = Master
IONODE = IOnode
CC = g++
FLAG = -O2

master:
	$(CC) $(FLAG) -c -o $(MASTER) $(MASTER).cpp
