CC = gcc
CFLAGS = -Wall

main: main.o
	$(CC) $(CFLAGS) -o main main.o
clean:
	rm -f *.o core.* main

main.o:
	$(CC) $(CFLAGS) -c main.c
