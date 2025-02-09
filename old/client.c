#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "interface.h"

#define TAILLE_BUFFER 1024    // Taille de la mémoire tampon pour les messages
#define PORT_SERVEUR 5000     // Port d'écoute du serveur

// Structure principale contenant l'état du jeu et les informations de connexion
typedef struct {
    int descripteur_socket;    // Descripteur du socket UDP
    volatile int en_cours;     // Flag indiquant si le jeu est en cours
    SDL_Window *fenetre;       // Pointeur vers la fenêtre SDL
    struct sockaddr_in adresse_serveur;  // Adresse du serveur
    pthread_mutex_t mutex;     // Mutex pour la synchronisation des threads
    
    // État du jeu
    int attente_adversaire;  // Indique si on attend un adversaire
    int ma_grille[10][10];     // Grille du joueur
    int grille_adversaire[10][10];  // Grille de l'adversaire
    int mon_tour;              // Indique si c'est le tour du joueur
} EtatJeu;

// Structure pour les messages échangés entre client et serveur
typedef struct {
    uint32_t id_message;      // Identifiant unique du message
    uint8_t type;             // Type de message (tir, résultat, etc.)
    uint8_t x;                // Coordonnée X sur la grille
    uint8_t y;                // Coordonnée Y sur la grille
    uint8_t resultat;         // Résultat de l'action (touché, manqué, etc.)
} MessageJeu;

// Thread gérant la réception des messages du serveur
void* thread_reception(void* arg) {
    EtatJeu* etat = (EtatJeu*)arg;
    char tampon[TAILLE_BUFFER];
    struct sockaddr_in adresse_origine;
    socklen_t taille_origine = sizeof(adresse_origine);
    MessageJeu* message;

    // Configuration du timeout sur le socket
    struct timeval delai;
    delai.tv_sec = 0;
    delai.tv_usec = 100000;  // 100ms
    setsockopt(etat->descripteur_socket, SOL_SOCKET, SO_RCVTIMEO, &delai, sizeof(delai));


    while(etat->en_cours) {
        int recu = recvfrom(etat->descripteur_socket, tampon, TAILLE_BUFFER, 0,
                           (struct sockaddr*)&adresse_origine, &taille_origine);
        
        if(recu > 0) {
            if(!etat->en_cours) break;
            message = (MessageJeu*)tampon;
            pthread_mutex_lock(&etat->mutex);
            
            switch(message->type) {
                case 1: // Réception d'un tir de l'adversaire
                    printf("Tir reçu aux coordonnées %d,%d\n", message->x, message->y);
                    break;
                    
                case 2: // Résultat de notre tir
                    printf("Résultat du tir: %s\n", message->resultat ? "Touché!" : "Manqué!");
                    etat->grille_adversaire[message->x][message->y] = message->resultat + 1;
                    etat->mon_tour = 1;
                    break;
            }
            
            pthread_mutex_unlock(&etat->mutex);
        }
    }

    printf("Thread de réception terminé\n");
    return NULL;
}

// Initialise la connexion avec le serveur
int initialiser_connexion(const char* hote, EtatJeu* etat) {
    struct hostent* ptr_hote;
    
    if ((ptr_hote = gethostbyname(hote)) == NULL) {
        perror("Erreur : impossible de trouver le serveur.");
        return 0;
    }

    // Création du socket UDP
    if ((etat->descripteur_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erreur : création socket impossible.");
        return 0;
    }

    // Configuration de l'adresse du serveur
    memset(&etat->adresse_serveur, 0, sizeof(etat->adresse_serveur));
    etat->adresse_serveur.sin_family = AF_INET;
    etat->adresse_serveur.sin_port = htons(PORT_SERVEUR);
    bcopy((char*)ptr_hote->h_addr, (char*)&etat->adresse_serveur.sin_addr, ptr_hote->h_length);

    return 1;
}

// Envoie un tir vers une position donnée
void envoyer_tir(EtatJeu* etat, int x, int y) {
    static uint32_t compteur_message = 0;
    MessageJeu msg = {
        .id_message = ++compteur_message,
        .type = 1,  // Type tir
        .x = x,
        .y = y
    };
    
    sendto(etat->descripteur_socket, &msg, sizeof(msg), 0,
           (struct sockaddr*)&etat->adresse_serveur, sizeof(etat->adresse_serveur));
    
    etat->mon_tour = 0;  // Attendre la réponse du serveur
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage : %s <adresse-serveur>\n", argv[0]);
        exit(1);
    }

    // Initialisation de l'état du jeu
    EtatJeu etat = {0};
    etat.en_cours = 1;
    pthread_mutex_init(&etat.mutex, NULL);

    // Établissement de la connexion avec le serveur
    if (!initialiser_connexion(argv[1], &etat)) {
        exit(1);
    }

    // Initialisation de l'interface graphique
    if (!initialiser_SDL()) {
        close(etat.descripteur_socket);
        pthread_mutex_destroy(&etat.mutex);
        exit(1);
    }

    // Création du thread de réception des messages
    pthread_t tid_reception;
    if (pthread_create(&tid_reception, NULL, thread_reception, &etat) != 0) {
        perror("Erreur création thread réception");
        close(etat.descripteur_socket);
        fermer_SDL();
        pthread_mutex_destroy(&etat.mutex);
        exit(1);
    }

    // Boucle principale du jeu
    SDL_Event evenement;
    int souris_x, souris_y;
    
    while (!etat.en_cours) {
        while (SDL_PollEvent(&evenement)) {
            if (evenement.type == SDL_QUIT) {
                printf("Demande de fermeture reçue\n");
                etat.en_cours = 0;
                break;
            }
            
            if(dessiner_fenetre_menu() == 1){
                etat.en_cours = 1;
                break;
            } else if(dessiner_fenetre_menu() == 2){
                etat.en_cours = 1;
                etat.attente_adversaire = 1;
                break;
            }
        }
    }

    while (etat.en_cours) {
        while (SDL_PollEvent(&evenement)) {
            if (evenement.type == SDL_QUIT) {
                printf("Demande de fermeture reçue\n");
                etat.en_cours = 0;
                break;
            }
            else if (evenement.type == SDL_MOUSEBUTTONDOWN && etat.mon_tour) {
                SDL_GetMouseState(&souris_x, &souris_y);
                // Conversion des coordonnées souris en coordonnées grille
                int grille_x = souris_x / TAILLE_CELLULE;
                int grille_y = souris_y / TAILLE_CELLULE;
                if (grille_x < 10 && grille_y < 10) {
                    envoyer_tir(&etat, grille_x, grille_y);
                }
            }
        }

        pthread_mutex_lock(&etat.mutex);
        
        // Mise à jour de l'affichage
        SDL_SetRenderDrawColor(rendu, 255, 255, 255, 255);
        SDL_RenderClear(rendu);


        // Dessin des grilles de jeu
        dessiner_grille();
        // TODO: Dessiner les bateaux et les tirs


        pthread_mutex_unlock(&etat.mutex);
        
        SDL_RenderPresent(rendu);
        SDL_Delay(16);  // ~60 FPS
    }

    printf("Nettoyage en cours...\n");
    etat.en_cours = 0;

    // Attente de la fin du thread de réception avec timeout
    struct timespec temps_max;
    clock_gettime(CLOCK_REALTIME, &temps_max);
    temps_max.tv_sec += 2;  // Timeout de 2 secondes
    int retour = pthread_timedjoin_np(tid_reception, NULL, &temps_max);
    if (retour != 0) {
        printf("Le thread de réception ne répond pas, terminaison forcée\n");
        pthread_cancel(tid_reception);
    }

    // Libération des ressources
    close(etat.descripteur_socket);
    pthread_mutex_destroy(&etat.mutex);
    fermer_SDL();

    printf("Programme terminé\n");
    return 0;
}