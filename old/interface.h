#ifndef INTERFACE_H
#define INTERFACE_H

#include <SDL2/SDL.h>

// Constantes de configuration de la fenêtre et de la grille
#define LARGEUR_FENETRE 800
#define HAUTEUR_FENETRE 600
#define TAILLE_GRILLE 10
#define TAILLE_CELLULE 40

// Variable globale pour le rendu
extern SDL_Renderer *rendu;

// Fonctions de l'interface graphique
int initialiser_SDL();             // Initialise la SDL et crée la fenêtre
void fermer_SDL();                 // Libère les ressources SDL
void dessiner_grille();           // Dessine la grille de jeu
void dessiner_bateau(int x, int y, int longueur, int horizontal);  // Dessine un bateau

#endif // INTERFACE_H