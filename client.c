#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "interface.h"
#include "message.h"

#define PORT 8888

int isValidPlacement(GameState* state, int x, int y, int length, int isHorizontal) {
    if (isHorizontal) {
        if (x + length > GRID_SIZE) return 0;
        for (int i = 0; i < length; i++) {
            if (state->grid[x + i][y] != 0) return 0;
            // Check adjacent cells
            for (int dy = -1; dy <= 1; dy++) {
                if (y + dy >= 0 && y + dy < GRID_SIZE) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int newX = x + i + dx;
                        if (newX >= 0 && newX < GRID_SIZE) {
                            if (state->grid[newX][y + dy] != 0) return 0;
                        }
                    }
                }
            }
        }
    } else {
        if (y + length > GRID_SIZE) return 0;
        for (int i = 0; i < length; i++) {
            if (state->grid[x][y + i] != 0) return 0;
            // Check adjacent cells
            for (int dx = -1; dx <= 1; dx++) {
                if (x + dx >= 0 && x + dx < GRID_SIZE) {
                    for (int dy = -1; dy <= 1; dy++) {
                        int newY = y + i + dy;
                        if (newY >= 0 && newY < GRID_SIZE) {
                            if (state->grid[x + dx][newY] != 0) return 0;
                        }
                    }
                }
            }
        }
    }
    return 1;
}

void placeShip(GameState* state, int x, int y, int length, int isHorizontal) {
    if (isHorizontal) {
        for (int i = 0; i < length; i++) {
            state->grid[x + i][y] = 1;
        }
    } else {
        for (int i = 0; i < length; i++) {
            state->grid[x][y + i] = 1;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <ip_serveur>\n", argv[0]);
        return 1;
    }

    // Initialize network
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_aton(argv[1], &server_addr.sin_addr);

    // Set socket to non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // Initialize game state
    GameState state = {0};
    state.placementPhase = 1;
    state.myTurn = 1;  // First player to connect starts

    // Initialize ships
    int shipLengths[] = {2,2,3,3,4,5};
    for (int i = 0; i < 6; i++) {
        state.ships[i].length = shipLengths[i];
        state.ships[i].isHorizontal = 1;
    }

    if (!initSDL()) return 1;

    SDL_Event event;
    int running = 1;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && !state.gameOver) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;

                if (state.placementPhase) {
                    // Convert mouse coordinates to grid coordinates
                    int gridX = (mouseX - GRID_OFFSET_X) / CELL_SIZE;
                    int gridY = (mouseY - GRID_OFFSET_Y) / CELL_SIZE;

                    if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
                        Ship* currentShip = &state.ships[state.currentShip];
                        if (isValidPlacement(&state, gridX, gridY, 
                                          currentShip->length, 
                                          currentShip->isHorizontal)) {
                            placeShip(&state, gridX, gridY, 
                                    currentShip->length, 
                                    currentShip->isHorizontal);
                            
                            // Send placement to server
                            Message msg = {0, gridX, gridY, currentShip->isHorizontal};
                            sendto(sock, &msg, sizeof(msg), 0,
                                  (struct sockaddr*)&server_addr, sizeof(server_addr));

                            state.currentShip++;
                            if (state.currentShip >= 6) {
                                state.placementPhase = 0;
                                printf("Phase de tir commencée\n");
                            }
                        }
                    }
                }
                else if (state.myTurn) {
                    // Handle shooting
                    int gridX = (mouseX - (GRID_OFFSET_X + 600)) / CELL_SIZE;
                    int gridY = (mouseY - GRID_OFFSET_Y) / CELL_SIZE;
                    if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE &&
                        state.enemyGrid[gridX][gridY] == 0) {
                        Message msg = {1, gridX, gridY, 0};
                        sendto(sock, &msg, sizeof(msg), 0,
                              (struct sockaddr*)&server_addr, sizeof(server_addr));
                        state.myTurn = 0;
                    }
                }
            }
            else if (event.type == SDL_KEYDOWN && state.placementPhase) {
                if (event.key.keysym.sym == SDLK_SPACE && state.currentShip < 6) {
                    state.ships[state.currentShip].isHorizontal = 
                        !state.ships[state.currentShip].isHorizontal;
                }
            }
        }

        // Check for network messages
        Message msg;
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        while (recvfrom(sock, &msg, sizeof(msg), 0,
                       (struct sockaddr*)&from_addr, &from_len) > 0) {
            switch (msg.type) {
                case 1:  // Shot result
                    state.enemyGrid[msg.x][msg.y] = msg.hit ? 1 : 2;
                    state.myTurn = 1;
                    break;
                case 2:  // Incoming shot
                    if (state.grid[msg.x][msg.y] == 1) {
                        state.grid[msg.x][msg.y] = 2;  // Hit
                    } else {
                        state.grid[msg.x][msg.y] = 3;  // Miss
                    }
                    break;
                case 3:  // Game over
                    state.gameOver = 1;
                    state.winner = msg.hit;  // Using hit field for winner ID
                    break;
            }
        }

        // Draw
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        drawGrid(GRID_OFFSET_X, "Mon plateau");
        drawGrid(GRID_OFFSET_X + 600, "Plateau de l'adversaire");

        if (state.placementPhase) {
            drawShips(&state);
            
            // Draw placement instructions
            SDL_Color textColor = {0, 0, 0, 255};
            char instructions[100];
            snprintf(instructions, 100, "Placez votre bateau de %d cases (ESPACE pour pivoter)", 
                    state.ships[state.currentShip].length);
            SDL_Surface* surface = TTF_RenderText_Solid(font, instructions, textColor);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect rect = {WINDOW_WIDTH/2 - surface->w/2, 10, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        } else {
            drawHitsAndMisses(&state);
            
            // Draw turn indicator
            SDL_Color textColor = {0, 0, 0, 255};
            const char* turnText = state.myTurn ? "C'est votre tour" : "Tour de l'adversaire";
            SDL_Surface* surface = TTF_RenderText_Solid(font, turnText, textColor);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect rect = {WINDOW_WIDTH/2 - surface->w/2, 10, surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }

        if (state.gameOver) {
            // Draw game over message
            SDL_Color textColor = {255, 0, 0, 255};
            const char* gameOverText = state.winner == state.myTurn ? 
                                     "Victoire !" : "Défaite...";
            SDL_Surface* surface = TTF_RenderText_Solid(font, gameOverText, textColor);
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_Rect rect = {WINDOW_WIDTH/2 - surface->w/2, 
                           WINDOW_HEIGHT/2 - surface->h/2,
                           surface->w, surface->h};
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_FreeSurface(surface);
            SDL_DestroyTexture(texture);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);  // ~60 FPS
    }

    close(sock);
    closeSDL();
    return 0;
}