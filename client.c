// client.c
#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "interface.h"

#define BUFFER_SIZE 1024
#define SERVER_PORT 5000

typedef struct {
    int socket_descriptor;
    int running;
    struct sockaddr_in server_addr;
    pthread_mutex_t mutex;
    // État du jeu
    int my_grid[10][10];
    int opponent_grid[10][10];
    int my_turn;
} GameState;

// Structure pour les messages du jeu
typedef struct {
    uint32_t message_id;  // Pour détecter les doublons
    uint8_t type;         // Type de message (tir, résultat, etc.)
    uint8_t x;           // Coordonnée X
    uint8_t y;           // Coordonnée Y
    uint8_t result;      // Résultat (touché, manqué, etc.)
} GameMessage;

// Thread pour recevoir les messages du serveur
void* receive_thread(void* arg) {
    GameState* state = (GameState*)arg;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    GameMessage* msg;

    while(state->running) {
        int received = recvfrom(state->socket_descriptor, buffer, BUFFER_SIZE, 0,
                              (struct sockaddr*)&from_addr, &from_len);
        
        if(received > 0) {
            msg = (GameMessage*)buffer;
            pthread_mutex_lock(&state->mutex);
            
            switch(msg->type) {
                case 1: // Tir reçu
                    printf("Tir reçu en %d,%d\n", msg->x, msg->y);
                    // Traiter le tir
                    break;
                    
                case 2: // Résultat de notre tir
                    printf("Résultat du tir: %s\n", msg->result ? "Touché!" : "Manqué!");
                    state->opponent_grid[msg->x][msg->y] = msg->result + 1;
                    state->my_turn = 1;
                    break;
            }
            
            pthread_mutex_unlock(&state->mutex);
        }
    }
    return NULL;
}

int init_connection(const char* host, GameState* state) {
    struct hostent* ptr_host;
    
    if ((ptr_host = gethostbyname(host)) == NULL) {
        perror("Erreur : impossible de trouver le serveur.");
        return 0;
    }

    // Création socket UDP
    if ((state->socket_descriptor = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erreur : création socket impossible.");
        return 0;
    }

    // Configuration adresse serveur
    memset(&state->server_addr, 0, sizeof(state->server_addr));
    state->server_addr.sin_family = AF_INET;
    state->server_addr.sin_port = htons(SERVER_PORT);
    bcopy((char*)ptr_host->h_addr, (char*)&state->server_addr.sin_addr, ptr_host->h_length);

    return 1;
}

// Envoyer un tir au serveur
void send_shot(GameState* state, int x, int y) {
    static uint32_t message_counter = 0;
    GameMessage msg = {
        .message_id = ++message_counter,
        .type = 1,  // Type tir
        .x = x,
        .y = y
    };
    
    sendto(state->socket_descriptor, &msg, sizeof(msg), 0,
           (struct sockaddr*)&state->server_addr, sizeof(state->server_addr));
    
    state->my_turn = 0;  // Attendre la réponse
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage : %s <adresse-serveur>\n", argv[0]);
        exit(1);
    }

    GameState state = {0};
    state.running = 1;
    pthread_mutex_init(&state.mutex, NULL);

    if (!init_connection(argv[1], &state)) {
        exit(1);
    }

    if (!init_SDL()) {
        close(state.socket_descriptor);
        pthread_mutex_destroy(&state.mutex);
        exit(1);
    }

    // Création thread réception
    pthread_t receive_tid;
    if (pthread_create(&receive_tid, NULL, receive_thread, &state) != 0) {
        perror("Erreur création thread réception");
        close(state.socket_descriptor);
        close_SDL();
        pthread_mutex_destroy(&state.mutex);
        exit(1);
    }

    // Boucle principale SDL
    SDL_Event e;
    int mouse_x, mouse_y;
    
    while (state.running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                state.running = 0;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && state.my_turn) {
                SDL_GetMouseState(&mouse_x, &mouse_y);
                // Convertir les coordonnées souris en coordonnées grille
                int grid_x = mouse_x / CELL_SIZE;
                int grid_y = mouse_y / CELL_SIZE;
                if (grid_x < 10 && grid_y < 10) {
                    send_shot(&state, grid_x, grid_y);
                }
            }
        }

        pthread_mutex_lock(&state.mutex);
        
        // Rendu
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Dessiner les grilles
        draw_grid();
        // TODO: Dessiner les bateaux et les tirs

        pthread_mutex_unlock(&state.mutex);
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    // Nettoyage
    pthread_join(receive_tid, NULL);
    close(state.socket_descriptor);
    pthread_mutex_destroy(&state.mutex);
    close_SDL();

    return 0;
}