CC = gcc
CFLAGS = -g -Wall -I..
LDFLAGS = -lpthread

all: echoclient echoserver

echoclient: echoclient.o ../csapp.o
	$(CC) $(CFLAGS) -o echoclient echoclient.o ../csapp.o $(LDFLAGS)

echoserver: echoserveri.o echo.o ../csapp.o
	$(CC) $(CFLAGS) -o echoserver echoserveri.o echo.o ../csapp.o $(LDFLAGS)

clean:
	rm -f *.o echoclient echoserver
