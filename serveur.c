#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "message.h"

#define PORT 5000
#define MAX_CLIENTS 2

typedef struct {
    struct sockaddr_in addr;
    int grid[10][10];
} Client;

volatile int running = 1;

void handle_sigint(int sig) {
    running = 0;
}

int main(void) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    
    bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    Client clients[MAX_CLIENTS] = {0};
    int num_clients = 0;

    printf("Serveur démarré sur le port %d\n", PORT);
    
    signal(SIGINT, handle_sigint);
    
    while (running) {
        Message msg;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        recvfrom(sock, &msg, sizeof(msg), 0,
                 (struct sockaddr*)&client_addr, &client_len);
        
        // Find or add client
        int client_id = -1;
        for (int i = 0; i < num_clients; i++) {
            if (clients[i].addr.sin_addr.s_addr == client_addr.sin_addr.s_addr &&
                clients[i].addr.sin_port == client_addr.sin_port) {
                client_id = i;
                break;
            }
        }
        
        if (client_id == -1 && num_clients < MAX_CLIENTS) {
            client_id = num_clients++;
            clients[client_id].addr = client_addr;
            memset(clients[client_id].grid, 0, sizeof(clients[client_id].grid));
            printf("Nouveau client connecté (%d/2)\n", num_clients);
        }
        
        if (client_id != -1) {
            if (msg.type == 0) {  // Ship placement
                // Place ship on grid
                int x = msg.x;
                int y = msg.y;
                int isHorizontal = msg.hit;  // Using hit field for orientation
                int length;
                
                // Determine ship length based on placement order
                int numShips = 0;
                for (int i = 0; i < 10; i++) {
                    for (int j = 0; j < 10; j++) {
                        if (clients[client_id].grid[i][j] == 1) numShips++;
                    }
                }
                
                if (numShips < 4) length = 2;        // First two ships: length 2
                else if (numShips < 10) length = 3;  // Next two ships: length 3
                else if (numShips < 14) length = 4;  // Next ship: length 4
                else length = 5;                     // Last ship: length 5
                
                // Place ship
                if (isHorizontal) {
                    for (int i = 0; i < length; i++) {
                        clients[client_id].grid[x + i][y] = 1;
                    }
                } else {
                    for (int i = 0; i < length; i++) {
                        clients[client_id].grid[x][y + i] = 1;
                    }
                }
            }
            else if (msg.type == 1 && num_clients == 2) {  // Shot
                int target_id = 1 - client_id;  // Other player
                int hit = 0;
                
                // Check if hit
                if (clients[target_id].grid[msg.x][msg.y] == 1) {
                    hit = 1;
                    clients[target_id].grid[msg.x][msg.y] = 2;  // Mark as hit
                }
                
                // Send result back to shooting player
                Message response = {1, msg.x, msg.y, hit};
                sendto(sock, &response, sizeof(response), 0,
                      (struct sockaddr*)&client_addr, client_len);
                
                // Send shot info to target player
                Message notification = {2, msg.x, msg.y, hit};  // Type 2 = incoming shot
                sendto(sock, &notification, sizeof(notification), 0,
                      (struct sockaddr*)&clients[target_id].addr, 
                      sizeof(clients[target_id].addr));
                
                // Check for game over (all ships sunk)
                int remaining_ships = 0;
                for (int i = 0; i < 10; i++) {
                    for (int j = 0; j < 10; j++) {
                        if (clients[target_id].grid[i][j] == 1) {
                            remaining_ships++;
                        }
                    }
                }
                
                if (remaining_ships == 0) {
                    printf("Joueur %d a gagné!\n", client_id + 1);
                    Message gameOver = {3, 0, 0, client_id};  // Type 3 = game over
                    for (int i = 0; i < num_clients; i++) {
                        sendto(sock, &gameOver, sizeof(gameOver), 0,
                              (struct sockaddr*)&clients[i].addr, 
                              sizeof(clients[i].addr));
                    }
                    // Reset game state
                    num_clients = 0;
                }
            }
        }
    }
    
    close(sock);
    printf("\nServeur arrêté proprement\n");
    return 0;
}