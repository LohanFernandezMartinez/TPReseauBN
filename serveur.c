#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "interface.h"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define PORT 5000

typedef struct {
    struct sockaddr_in addr;
    int grid[10][10];
    uint32_t last_message_id;
    time_t last_seen;
} Client;

typedef struct {
    int socket_descriptor;
    int running;
    int nb_clients;
    Client clients[MAX_CLIENTS];
    pthread_mutex_t mutex;
} ServerState;

typedef struct {
    uint32_t message_id;
    uint8_t type;
    uint8_t x;
    uint8_t y;
    uint8_t result;
} GameMessage;

// Trouver ou ajouter un client
int find_or_add_client(ServerState* state, struct sockaddr_in* addr) {
    int i;
    for(i = 0; i < state->nb_clients; i++) {
        if(state->clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
           state->clients[i].addr.sin_port == addr->sin_port) {
            state->clients[i].last_seen = time(NULL);
            return i;
        }
    }
    
    if(state->nb_clients < MAX_CLIENTS) {
        i = state->nb_clients++;
        state->clients[i].addr = *addr;
        state->clients[i].last_seen = time(NULL);
        memset(state->clients[i].grid, 0, sizeof(state->clients[i].grid));
        state->clients[i].last_message_id = 0;
        return i;
    }
    
    return -1;
}

// Thread pour nettoyer les clients inactifs
void* cleanup_thread(void* arg) {
    ServerState* state = (ServerState*)arg;
    while(state->running) {
        sleep(5);  // Vérifier toutes les 5 secondes
        
        pthread_mutex_lock(&state->mutex);
        time_t now = time(NULL);
        
        for(int i = 0; i < state->nb_clients; i++) {
            if(now - state->clients[i].last_seen > 30) {  // 30 secondes timeout
                printf("Client %d timeout\n", i);
                // Déplacer le dernier client à cette position
                if(i < state->nb_clients - 1) {
                    state->clients[i] = state->clients[state->nb_clients - 1];
                }
                state->nb_clients--;
                i--;  // Revérifier cette position
            }
        }
        
        pthread_mutex_unlock(&state->mutex);
    }
    return NULL;
}

int init_server(ServerState* state) {
    struct sockaddr_in addr;
    
    // Création socket UDP
    if ((state->socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erreur création socket");
        return 0;
    }

    // Configuration adresse
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    // Bind
    if (bind(state->socket_descriptor, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Erreur bind");
        return 0;
    }

    return 1;
}

int main(void) {
    ServerState state = {0};
    state.running = 1;
    pthread_mutex_init(&state.mutex, NULL);

    if (!init_server(&state)) {
        exit(1);
    }

    if (!init_SDL()) {
        close(state.socket_descriptor);
        pthread_mutex_destroy(&state.mutex);
        exit(1);
    }

    // Thread pour nettoyer les clients inactifs
    pthread_t cleanup_tid;
    pthread_create(&cleanup_tid, NULL, cleanup_thread, &state);

    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    GameMessage* msg;
    GameMessage response;

    // Boucle principale
    while(state.running) {
        // Gestion des événements SDL
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                state.running = 0;
            }
        }

        // Réception des messages (non-bloquant)
        struct timeval tv = {0, 0};  // Polling immédiat
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(state.socket_descriptor, &readfds);
        
        if (select(state.socket_descriptor + 1, &readfds, NULL, NULL, &tv) > 0) {
            int received = recvfrom(state.socket_descriptor, buffer, BUFFER_SIZE, 0,
                                  (struct sockaddr*)&client_addr, &client_len);
            
            if(received > 0) {
                pthread_mutex_lock(&state.mutex);
                
                int client_id = find_or_add_client(&state, &client_addr);
                if(client_id >= 0) {
                    msg = (GameMessage*)buffer;
                    
                    // Éviter les doublons
                    if(msg->message_id > state.clients[client_id].last_message_id) {
                        state.clients[client_id].last_message_id = msg->message_id;
                        
                        // Traiter le message
                        switch(msg->type) {
                            case 1:  // Tir
                                printf("Tir reçu du client %d en %d,%d\n", 
                                       client_id, msg->x, msg->y);
                                
                                // Envoyer la réponse
                                response.message_id = msg->message_id;
                                response.type = 2;
                                response.x = msg->x;
                                response.y = msg->y;
                                response.result = 0;  // 0 = manqué, 1 = touché
                                
                                sendto(state.socket_descriptor, &response, sizeof(response), 0,
                                      (struct sockaddr*)&client_addr, client_len);
                                break;
                        }
                    }
                }
                
                pthread_mutex_unlock(&state.mutex);
            }
        }

        // Rendu SDL
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        
        pthread_mutex_lock(&state.mutex);
        // TODO: Afficher l'état des parties en cours
        pthread_mutex_unlock(&state.mutex);
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Nettoyage
    pthread_join(cleanup_tid, NULL);
    close(state.socket_descriptor);
    pthread_mutex_destroy(&state.mutex);
    close_SDL();

    return 0;
}