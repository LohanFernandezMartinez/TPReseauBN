#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "game.h"
#include "message.h"

#define PORT 5000

GameState clientState;
int enemyGrid[GRID_SIZE][GRID_SIZE] = {0};
int myTurn = 0;

void placeShipsManually(GameState* state) {
    int shipLengths[] = {5, 4, 3, 3, 2};
    printf("\nPlacement des bateaux\n");
    
    for (int i = 0; i < NUM_SHIPS; i++) {
        printf("\nPlacement d'un bateau de longueur %d\n", shipLengths[i]);
        displayGrid(state->grid, 1);
        
        int placed = 0;
        while (!placed) {
            int x, y, orientation;
            printf("Entrez les coordonnées (x y orientation[0=vertical,1=horizontal]): ");
            scanf("%d %d %d", &x, &y, &orientation);
            
            if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE &&
                isValidPlacement(state, x, y, shipLengths[i], orientation)) {
                placeShip(state, x, y, shipLengths[i], orientation, i);
                placed = 1;
            } else {
                printf("Position invalide, réessayez.\n");
            }
        }
    }
}

void choose_game_mode(int sock) {
    int choice;
    Message msg;
    
    printf("Choisissez le mode de jeu :\n");
    printf("1. Solo (contre le serveur)\n");
    printf("2. Multijoueur\n");
    printf("Votre choix (1-2) : ");
    scanf("%d", &choice);
    
    msg.type = MSG_MODE_CHOICE;
    msg.data = (choice == 2) ? 1 : 0;
    send(sock, &msg, sizeof(Message), 0);
}

void getShot(int* x, int* y) {
    do {
        printf("Entrez les coordonnées de tir (x y): ");
        scanf("%d %d", x, y);
    } while (*x < 0 || *x >= GRID_SIZE || *y < 0 || *y >= GRID_SIZE || 
             enemyGrid[*x][*y] != 0);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <adresse_serveur>\n", argv[0]);
        return 1;
    }
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    Message msg;
    
    // Configuration et connexion au serveur
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connexion échouée\n");
        return -1;
    }
    
    printf("Connecté au serveur\n");
    
    // Choix du mode de jeu
    choose_game_mode(sock);
    
    // Réception de la confirmation du serveur
    recv(sock, &msg, sizeof(Message), 0);

    if (msg.type == MSG_WAIT) {
        printf("En attente d'un autre joueur...\n");
        recv(sock, &msg, sizeof(Message), 0);
    }
    
    if (msg.type == MSG_GAME_START) {
        int multiplayer = msg.data;
        
        // Placement des bateaux
        initGameState(&clientState);
        placeShipsManually(&clientState);
        
        // Informer le serveur que le placement est terminé
        msg.type = MSG_PLACEMENT_DONE;
        send(sock, &msg, sizeof(Message), 0);
        
        if (multiplayer) {
            printf("En attente que l'autre joueur place ses bateaux...\n");
            recv(sock, &msg, sizeof(Message), 0);
        }
        
        // Réception du premier tour
        recv(sock, &msg, sizeof(Message), 0);
        if(msg.type == MSG_TURN && msg.data == 1)
            myTurn = 1;
        else
            myTurn = 0;
        
        // Boucle principale du jeu
        while (1) {
            printf("\nVotre grille :\n");
            displayGrid(clientState.grid, 1);
            printf("\nGrille ennemie :\n");
            displayGrid(enemyGrid, 0);
            
            if (myTurn) {
                printf("\nC'est votre tour !\n");
                int x, y;
                getShot(&x, &y);
                
                msg.type = MSG_SHOT;
                msg.x = x;
                msg.y = y;
                send(sock, &msg, sizeof(Message), 0);
                
            }
            else {
                printf("\nEn attente du tour de l'adversaire...\n");
            }
            
            // Attente des messages
            recv(sock, &msg, sizeof(Message), 0);
            switch (msg.type) {
                case MSG_SHOT:
                    printf("Tir adverse en (%d,%d)\n", msg.x, msg.y);
                    int hit = processShot(&clientState, msg.x, msg.y);
                    msg.type = MSG_RESULT;
                    msg.data = hit;
                    if (clientState.shipsLeft == 0) msg.data = 2;  // Victoire
                    send(sock, &msg, sizeof(Message), 0);
                    break;
                    
                case MSG_RESULT:
                    if (msg.data == 0) {
                        printf("Manqué !\n");
                        enemyGrid[msg.x][msg.y] = 3;
                    }
                    else if (msg.data == 1) {
                        printf("Touché !\n");
                        enemyGrid[msg.x][msg.y] = 2;
                    }
                    else if (msg.data == 2) {
                        printf("Victoire ! Tous les bateaux ennemis sont coulés !\n");
                        return 0;
                    }
                    break;
                    
                case MSG_TURN:
                    if (multiplayer)
                        myTurn = 1 - myTurn;
                    else
                        myTurn = 1;
                    break;
                    
                case MSG_GAME_OVER:
                    printf("Partie terminée !\n");
                    return 0;
            }
        }
    }
    
    close(sock);
    return 0;
}