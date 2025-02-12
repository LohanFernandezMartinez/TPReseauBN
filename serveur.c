#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include "game.h"
#include "message.h"

#define PORT 5000
#define MAX_CLIENTS 2

typedef struct {
    int socket;
    int id;
    GameState state;
    int mode;  // 0 = solo, 1 = multiplayer
    int ready;  // Pour la synchronisation du placement des bateaux
} Client;

typedef struct {
    Client* clients[2];
    int currentTurn;
    int gameStarted;
} MultiplayerGame;

Client* clients[MAX_CLIENTS] = {NULL};
int num_clients = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
MultiplayerGame* waiting_game = NULL;

void broadcast_to_opponent(Client* sender, Message* msg) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i] != sender && clients[i]->mode == 1) {
            send(clients[i]->socket, msg, sizeof(Message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void* handle_client(void* arg) {
    Client* client = (Client*)arg;
    Message msg;
    GameState serverState;
    
    // Attendre le choix du mode de jeu
    recv(client->socket, &msg, sizeof(Message), 0);
    client->mode = msg.data;
    
    if (client->mode == 0) {  // Mode solo
        // Initialiser le jeu solo
        initGameState(&serverState);
        placeRandomShips(&serverState);
        printf("Serveur: Bateaux placés\n");
        displayGrid(serverState.grid, 1);

        // Envoyer confirmation au client
        msg.type = MSG_GAME_START;
        msg.data = 0;  // Mode solo
        send(client->socket, &msg, sizeof(Message), 0);
        
        // Boucle de jeu solo
        // Boucle principale du jeu
        int gameOver = 0;
        while (!gameOver) {
            // Réception du message du client
            if (recv(client->socket, &msg, sizeof(Message), 0) <= 0) {
                printf("Client déconnecté\n");
                break;
            }
            
            switch (msg.type) {
                case MSG_SHOT: {
                    printf("Tir reçu en (%d,%d)\n", msg.x, msg.y);
                    
                    // Traitement du tir
                    int result = processShot(&serverState, msg.x, msg.y);
                    
                    // Envoi du résultat
                    Message response = {
                        .type = MSG_RESULT,
                        .x = msg.x,
                        .y = msg.y,
                        .data = result
                    };
                    send(client->socket, &response, sizeof(Message), 0);
                    
                    // Vérification de fin de partie
                    if (serverState.shipsLeft == 0) {
                        Message gameOverMsg = {
                            .type = MSG_GAME_OVER,
                            .data = 1  // Victoire du client
                        };
                        send(client->socket, &gameOverMsg, sizeof(Message), 0);
                        gameOver = 1;
                    }
                    break;
                }

                case MSG_PLACEMENT_DONE: {
                    printf("Placement des bateaux terminé\nLa partie va débuter\n");
                    msg.type = MSG_TURN;
                    msg.data = 1;  // C'est au client de jouer
                    send(client->socket, &msg, sizeof(Message), 0);
                    break;
                }
            }
        }
    }
    else {  // Mode multijoueur
        pthread_mutex_lock(&clients_mutex);
        if (!waiting_game) {
            // Premier joueur, créer une nouvelle partie
            waiting_game = malloc(sizeof(MultiplayerGame));
            waiting_game->clients[0] = client;
            waiting_game->gameStarted = 0;
            waiting_game->currentTurn = -1;  // Aucun tour assigné pour l'instant
            
            // Informer le client qu'il doit attendre
            msg.type = MSG_WAIT;
            send(client->socket, &msg, sizeof(Message), 0);
        }
        else {
            // Deuxième joueur, commencer la partie
            waiting_game->clients[1] = client;
            
            // Informer les deux clients que la partie commence
            msg.type = MSG_GAME_START;
            msg.data = 1;  // Mode multijoueur
            send(waiting_game->clients[0]->socket, &msg, sizeof(Message), 0);
            send(waiting_game->clients[1]->socket, &msg, sizeof(Message), 0);
        }
        pthread_mutex_unlock(&clients_mutex);
        
        // Boucle de jeu multijoueur
        while (1) {
            if (recv(client->socket, &msg, sizeof(Message), 0) <= 0) {
                break;
            }
            
            switch (msg.type) {
                case MSG_PLACEMENT_DONE:
                    client->ready = 1;
                    if (waiting_game->clients[0]->ready && waiting_game->clients[1]->ready) {
                        // Les deux joueurs ont placé leurs bateaux
                        msg.type = MSG_ALL_PLACED;
                        send(waiting_game->clients[0]->socket, &msg, sizeof(Message), 0);
                        send(waiting_game->clients[1]->socket, &msg, sizeof(Message), 0);
                        
                        // Initialiser le premier tour
                        if (waiting_game->currentTurn == -1) {
                            waiting_game->currentTurn = 0;  // Le premier joueur commence
                            msg.type = MSG_TURN;
                            
                            // Envoyer les messages de tour aux deux joueurs
                            msg.data = 1;  // C'est ton tour
                            send(waiting_game->clients[0]->socket, &msg, sizeof(Message), 0);
                            msg.data = 0;  // Ce n'est pas ton tour
                            send(waiting_game->clients[1]->socket, &msg, sizeof(Message), 0);
                        }
                    }
                    break;
                    
                case MSG_SHOT:
                    if (waiting_game->currentTurn == client->id) {

                        printf("--> Tir reçu en (%d,%d) du client %d au client %d -->\n", msg.x, msg.y, client->id, 1 - client->id);
                        // Transmettre le tir à l'adversaire
                        broadcast_to_opponent(client, &msg);
                        waiting_game->currentTurn = 1 - waiting_game->currentTurn;
                        

                        // Informer les joueurs du changement de tour
                        msg.type = MSG_TURN;
                        msg.data = waiting_game->currentTurn;           // Ne sert à rien ?
                        send(waiting_game->clients[0]->socket, &msg, sizeof(Message), 0);
                        send(waiting_game->clients[1]->socket, &msg, sizeof(Message), 0);
                    }
                    break;
                    
                case MSG_RESULT:
                    broadcast_to_opponent(client, &msg);
                    if (msg.data == 2) {  // Victoire
                        msg.type = MSG_GAME_OVER;
                        send(waiting_game->clients[0]->socket, &msg, sizeof(Message), 0);
                        send(waiting_game->clients[1]->socket, &msg, sizeof(Message), 0);
                        return NULL;
                    }
                    break;

                case MSG_WAIT:
                    break;

            }
        }
    }
    
    return NULL;
}

void placeRandomShips(GameState* state) {
    int shipLengths[] = {5, 4, 3, 3, 2};
    srand(time(NULL));
    
    for (int i = 0; i < NUM_SHIPS; i++) {
        int placed = 0;
        while (!placed) {
            int x = rand() % GRID_SIZE;
            int y = rand() % GRID_SIZE;
            int isHorizontal = rand() % 2;
            
            if (isValidPlacement(state, x, y, shipLengths[i], isHorizontal)) {
                placeShip(state, x, y, shipLengths[i], isHorizontal, i);
                placed = 1;
            }
        }
    }
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    pthread_t thread_id;
    
    // Initialisation du serveur
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    
    printf("Serveur démarré sur le port %d\n", PORT);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        printf("Client connecté\n");
        
        Client* client = malloc(sizeof(Client));
        client->socket = client_socket;
        client->id = num_clients++;
        client->ready = 0;
        initGameState(&client->state);
        
        pthread_create(&thread_id, NULL, handle_client, client);
        pthread_detach(thread_id);
    }
    
    return 0;
}