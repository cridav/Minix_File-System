
CC=cc
CFLAGS=-o

#all: fileSystem.c

file_system: fileSystem.c 
	$(CC) $(CFLAGS) execfile fileSystem.c

clean:
	rm -f *.o
