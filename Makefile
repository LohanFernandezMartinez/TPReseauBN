CC = gcc
CFLAGS = -Wall -Wextra -I/usr/include/SDL2
LDFLAGS = -lSDL2

CLIENT_SRC = client.c interface.c
SERVER_SRC = serveur.c interface.c

CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

all: client serveur

client: client.o interface.o
	$(CC) -o client client.o interface.o $(LDFLAGS)

serveur: serveur.o interface.o
	$(CC) -o serveur serveur.o interface.o $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o client serveur