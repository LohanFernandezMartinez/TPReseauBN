#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "interface.h"

#define PORT 8888

typedef struct {
    uint8_t type;  // 0=placement, 1=shot
    uint8_t x;
    uint8_t y;
    uint8_t hit;
} Message;

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
    
    // Initialize game state
    GameState state = {0};
    state.placementPhase = 1;
    
    // Initialize ships
    int shipLengths[] = {2,2,3,3,4,5};
    for (int i = 0; i < 6; i++) {
        state.ships[i].length = shipLengths[i];
        state.ships[i].isHorizontal = 1;
    }
    
    if (!initSDL()) {
        printf("Erreur initialisation SDL\n");
        return 1;
    }
    printf("SDL initialisée avec succès\n");
    
    SDL_Event event;
    int running = 1;
    
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                int mouseX = event.button.x;
                int mouseY = event.button.y;
                
                if (state.placementPhase) {
                    if (handleShipPlacement(&state, mouseX, mouseY)) {
                        Message msg = {0, state.ships[state.currentShip-1].x,
                                     state.ships[state.currentShip-1].y, 
                                     state.ships[state.currentShip-1].isHorizontal};
                        sendto(sock, &msg, sizeof(msg), 0,
                              (struct sockaddr*)&server_addr, sizeof(server_addr));
                    }
                }
                else if (state.myTurn) {
                    // Handle shooting
                    int gridX = (mouseX - (GRID_OFFSET_X + 600)) / CELL_SIZE;
                    int gridY = (mouseY - GRID_OFFSET_Y) / CELL_SIZE;
                    if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
                        Message msg = {1, gridX, gridY, 0};
                        sendto(sock, &msg, sizeof(msg), 0,
                              (struct sockaddr*)&server_addr, sizeof(server_addr));
                        state.myTurn = 0;
                    }
                }
            }
            else if (event.type == SDL_KEYDOWN && state.placementPhase) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    rotateCurrentShip(&state);
                }
            }
        }
        
        // Check for network messages
        Message msg;
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);
        if (recvfrom(sock, &msg, sizeof(msg), MSG_DONTWAIT,
                     (struct sockaddr*)&from_addr, &from_len) > 0) {
            if (msg.type == 1) {  // Shot result
                state.enemyGrid[msg.x][msg.y] = msg.hit ? 1 : 2;
                state.myTurn = 1;
            }
        }
        
        // Draw
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        
        drawGrid(GRID_OFFSET_X, "Mon plateau");
        drawGrid(GRID_OFFSET_X + 600, "Plateau de l'adversaire");
        
        if (state.placementPhase) {
            drawShips(&state);
        } else {
            drawHitsAndMisses(&state);
        }
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    close(sock);
    closeSDL();
    return 0;
}