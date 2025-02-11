#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

// Types de messages
#define MSG_MODE_CHOICE 0    // Choix du mode de jeu
#define MSG_WAIT 1          // Attente d'un autre joueur
#define MSG_GAME_START 2    // Début de partie
#define MSG_PLACEMENT_DONE 3 // Placement des bateaux terminé
#define MSG_ALL_PLACED 4    // Tous les bateaux sont placés
#define MSG_SHOT 5          // Tir
#define MSG_RESULT 6        // Résultat du tir
#define MSG_TURN 7          // Changement de tour
#define MSG_GAME_OVER 8     // Fin de partie

typedef struct {
    uint8_t type;    // Type de message
    uint8_t x;       // Coordonnée X
    uint8_t y;       // Coordonnée Y
    uint8_t data;    // Données supplémentaires
} Message;

#endif