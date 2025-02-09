CC = gcc
CFLAGS = -Wall -Wextra -I/usr/include/SDL2
LDFLAGS = -lSDL2 -lSDL2_ttf -pthread

# Source files
CLIENT_SRC = client.c interface.c
SERVER_SRC = serveur.c
COMMON_HEADERS = interface.h message.h

# Object files
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

# Targets
all: client serveur

# Client compilation
client: $(CLIENT_OBJ)
	$(CC) -o $@ $(CLIENT_OBJ) $(LDFLAGS) -D_GNU_SOURCE

# Server compilation
serveur: $(SERVER_OBJ)
	$(CC) -o $@ $(SERVER_OBJ) $(LDFLAGS) -D_GNU_SOURCE

# Object file compilation
%.o: %.c $(COMMON_HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) client serveur

# Dependencies
client.o: client.c interface.h message.h
interface.o: interface.c interface.h
serveur.o: serveur.c message.h

.PHONY: all clean