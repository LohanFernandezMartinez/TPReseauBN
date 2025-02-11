CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -pthread

# Source files
CLIENT_SRC = client.c game.c
SERVER_SRC = serveur.c game.c
COMMON_HEADERS = game.h message.h

# Object files
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

# Targets
all: client serveur

# Client compilation
client: $(CLIENT_OBJ)
	$(CC) -o $@ $(CLIENT_OBJ) $(LDFLAGS)

# Server compilation
serveur: $(SERVER_OBJ)
	$(CC) -o $@ $(SERVER_OBJ) $(LDFLAGS)

# Object file compilation
%.o: %.c $(COMMON_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f *.o client serveur

# Dependencies
client.o: client.c game.h message.h
game.o: game.c game.h
serveur.o: serveur.c game.h message.h

.PHONY: all clean