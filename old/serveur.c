#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "interface.h"

#define MAX_CLIENTS 2
#define TAILLE_BUFFER 1024
#define PORT 5000

// Structure représentant un client connecté
typedef struct {
    struct sockaddr_in adresse;     // Adresse réseau du client
    int grille[10][10];            // Grille de jeu du client
    uint32_t dernier_message_id;    // ID du dernier message reçu
    time_t derniere_activite;       // Horodatage de la dernière activité
} Client;

// Structure principale du serveur
typedef struct {
    int descripteur_socket;          // Socket d'écoute
    int en_cours;                    // État du serveur
    int nb_clients;                  // Nombre de clients connectés
    Client clients[MAX_CLIENTS];     // Tableau des clients
    pthread_mutex_t mutex;           // Mutex pour la synchronisation
} EtatServeur;

// Structure des messages échangés
typedef struct {
    uint32_t id_message;            // Identifiant unique du message
    uint8_t type;                   // Type de message
    uint8_t x;                      // Coordonnée X
    uint8_t y;                      // Coordonnée Y
    uint8_t resultat;               // Résultat de l'action
} MessageJeu;

// Trouve ou ajoute un client dans la liste
int trouver_ou_ajouter_client(EtatServeur* etat, struct sockaddr_in* adresse) {
    int i;
    // Recherche d'un client existant
    for(i = 0; i < etat->nb_clients; i++) {
        if(etat->clients[i].adresse.sin_addr.s_addr == adresse->sin_addr.s_addr &&
           etat->clients[i].adresse.sin_port == adresse->sin_port) {
            etat->clients[i].derniere_activite = time(NULL);
            return i;
        }
    }
    
    // Ajout d'un nouveau client si possible
    if(etat->nb_clients < MAX_CLIENTS) {
        i = etat->nb_clients++;
        etat->clients[i].adresse = *adresse;
        etat->clients[i].derniere_activite = time(NULL);
        memset(etat->clients[i].grille, 0, sizeof(etat->clients[i].grille));
        etat->clients[i].dernier_message_id = 0;
        return i;
    } else {
        printf("Trop de clients connectés\n");
    }
    
    return -1;
}

// Thread de nettoyage des clients inactifs
void* thread_nettoyage(void* arg) {
    EtatServeur* etat = (EtatServeur*)arg;
    while(etat->en_cours) {
        sleep(5);  // Vérification toutes les 5 secondes
        
        pthread_mutex_lock(&etat->mutex);
        time_t maintenant = time(NULL);
        
        // Parcours des clients pour détecter l'inactivité
        for(int i = 0; i < etat->nb_clients; i++) {
            if(maintenant - etat->clients[i].derniere_activite > 120) {  // Timeout de 120 secondes
                printf("Client %d déconnecté (timeout)\n", i);
                // Déplacement du dernier client à cette position
                if(i < etat->nb_clients - 1) {
                    etat->clients[i] = etat->clients[etat->nb_clients - 1];
                }
                etat->nb_clients--;
                i--;  // Revérifier cette position
            }
        }
        
        pthread_mutex_unlock(&etat->mutex);
    }
    return NULL;
}

// Initialisation du serveur
int initialiser_serveur(EtatServeur* etat) {
    struct sockaddr_in adresse;
    
    // Création du socket UDP
    if ((etat->descripteur_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erreur création socket");
        return 0;
    }

    // Configuration de l'adresse d'écoute
    memset(&adresse, 0, sizeof(adresse));
    adresse.sin_family = AF_INET;
    adresse.sin_addr.s_addr = htonl(INADDR_ANY);
    adresse.sin_port = htons(PORT);

    // Association du socket à l'adresse
    if (bind(etat->descripteur_socket, (struct sockaddr*)&adresse, sizeof(adresse)) < 0) {
        perror("Erreur bind");
        return 0;
    }

    return 1;
}

int main(void) {
    // Initialisation de l'état du serveur
    EtatServeur etat = {0};
    etat.en_cours = 1;
    pthread_mutex_init(&etat.mutex, NULL);

    // Initialisation du serveur et de l'interface graphique
    if (!initialiser_serveur(&etat)) {
        exit(1);
    }

    if (!initialiser_SDL()) {
        close(etat.descripteur_socket);
        pthread_mutex_destroy(&etat.mutex);
        exit(1);
    }

    // Démarrage du thread de nettoyage des clients inactifs
    pthread_t tid_nettoyage;
    pthread_create(&tid_nettoyage, NULL, thread_nettoyage, &etat);

    // Variables pour la boucle principale
    char tampon[TAILLE_BUFFER];
    struct sockaddr_in adresse_client;
    socklen_t taille_client = sizeof(adresse_client);
    MessageJeu* message;
    MessageJeu reponse;

    // Boucle principale du serveur
    while(etat.en_cours) {
        // Gestion des événements SDL
        SDL_Event evenement;
        while (SDL_PollEvent(&evenement)) {
            if (evenement.type == SDL_QUIT) {
                etat.en_cours = 0;
            }
        }

        // Réception des messages (non-bloquant)
        struct timeval delai = {0, 0}; // Délai nul pour le polling
        fd_set sockets_lecture;
        FD_ZERO(&sockets_lecture);
        FD_SET(etat.descripteur_socket, &sockets_lecture);
        
        if (select(etat.descripteur_socket + 1, &sockets_lecture, NULL, NULL, &delai) > 0) {
            int octets_recus = recvfrom(etat.descripteur_socket, tampon, TAILLE_BUFFER, 0,
                                      (struct sockaddr*)&adresse_client, &taille_client);
            
            if(octets_recus > 0) {
                pthread_mutex_lock(&etat.mutex);
                
                // Identifier ou créer le client
                int id_client = trouver_ou_ajouter_client(&etat, &adresse_client);
                if(id_client >= 0) {
                    message = (MessageJeu*)tampon;
                    
                    // Éviter le traitement des messages dupliqués
                    if(message->id_message > etat.clients[id_client].dernier_message_id) {
                        etat.clients[id_client].dernier_message_id = message->id_message;
                        
                        // Traitement selon le type de message
                        switch(message->type) {
                            case 1:  // Message de type tir
                                printf("Tir reçu du client %d aux coordonnées %d,%d\n", 
                                       id_client, message->x, message->y);
                                
                                // Préparation de la réponse
                                reponse.id_message = message->id_message;
                                reponse.type = 2;  // Type réponse au tir
                                reponse.x = message->x;
                                reponse.y = message->y;
                                reponse.resultat = 0;  // 0 = manqué, 1 = touché
                                
                                // Envoi de la réponse au client
                                sendto(etat.descripteur_socket, &reponse, sizeof(reponse), 0,
                                     (struct sockaddr*)&adresse_client, taille_client);
                                break;
                                
                            // Autres types de messages à traiter ici
                        }
                    }
                }
                
                pthread_mutex_unlock(&etat.mutex);
            }
        }

        // Mise à jour de l'affichage
        SDL_SetRenderDrawColor(rendu, 255, 255, 255, 255);  // Fond blanc
        SDL_RenderClear(rendu);
        
        pthread_mutex_lock(&etat.mutex);
        // TODO: Afficher l'état des parties en cours
        // - Position des bateaux
        // - État des tirs
        // - Score des joueurs
        pthread_mutex_unlock(&etat.mutex);
        
        SDL_RenderPresent(rendu);
        SDL_Delay(16);  // ~60 FPS
    }

    // Nettoyage et fermeture propre
    printf("Arrêt du serveur...\n");
    
    // Attendre la fin du thread de nettoyage
    pthread_join(tid_nettoyage, NULL);
    
    // Libération des ressources
    close(etat.descripteur_socket);
    pthread_mutex_destroy(&etat.mutex);
    fermer_SDL();

    printf("Serveur arrêté\n");
    return 0;
}