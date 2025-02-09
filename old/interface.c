#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include "interface.h"

// Dimensions de la fenêtre et de la grille de jeu
#define LARGEUR_FENETRE 1000
#define HAUTEUR_FENETRE 600
#define TAILLE_GRILLE 10
#define TAILLE_CELLULE 20

// Variables globales pour l'interface graphique
SDL_Window *fenetre = NULL;
SDL_Renderer *rendu = NULL;

// Initialise la SDL et crée la fenêtre de jeu
int initialiser_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Impossible d'initialiser la SDL! Erreur: %s\n", SDL_GetError());
        return 0;
    }

    fenetre = SDL_CreateWindow("Bataille Navale", 
                             SDL_WINDOWPOS_UNDEFINED, 
                             SDL_WINDOWPOS_UNDEFINED, 
                             LARGEUR_FENETRE, 
                             HAUTEUR_FENETRE, 
                             SDL_WINDOW_SHOWN);
    if (fenetre == NULL) {
        printf("Impossible de créer la fenêtre! Erreur: %s\n", SDL_GetError());
        return 0;
    }

    rendu = SDL_CreateRenderer(fenetre, -1, SDL_RENDERER_ACCELERATED);
    if (rendu == NULL) {
        printf("Impossible de créer le rendu! Erreur: %s\n", SDL_GetError());
        SDL_DestroyWindow(fenetre);
        return 0;
    }

    return 1;
}

// Libère les ressources SDL
void fermer_SDL() {
    SDL_DestroyRenderer(rendu);
    SDL_DestroyWindow(fenetre);
    SDL_Quit();
}

int dessiner_fenetre_menu () {
    // Création de la fenêtre de menu
    SDL_Rect menu;
    menu.x = 0;
    menu.y = 0;
    menu.w = LARGEUR_FENETRE;
    menu.h = HAUTEUR_FENETRE;
    
    // Ajouter deux boutons pour les options Multiplayer et Singleplayer
    SDL_SetRenderDrawColor(rendu, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(rendu, &menu);
    SDL_RenderPresent(rendu);

    int choix = 0;

    // Attendre le clic de l'utilisateur
    SDL_Event evenement;
    int continuer = 1;
    while (continuer) {
        SDL_WaitEvent(&evenement);
        if (evenement.type == SDL_MOUSEBUTTONDOWN) {
            if (evenement.button.button == SDL_BUTTON_LEFT) {
                if (evenement.button.x >= 100 && evenement.button.x <= 100 &&
                    evenement.button.y >= 100 && evenement.button.y <= 100) {
                    // Lancer le mode Singleplayer
                    choix = 1;
                    continuer = 0;
                }
                else if (evenement.button.x >= 100 && evenement.button.x <= 100 &&
                         evenement.button.y >= 200 && evenement.button.y <= 200) {
                    // Lancer le mode Multiplayer
                    choix = 2;
                    continuer = 0;
                }
            }
        }
    }

    // Nettoyer la fenêtre de menu
    SDL_SetRenderDrawColor(rendu, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(rendu);
    SDL_RenderPresent(rendu);

    return choix;
}

// Dessine la grille de jeu
void dessiner_grille() {
    // Définir la couleur des lignes en noir
    SDL_SetRenderDrawColor(rendu, 0, 0, 0, SDL_ALPHA_OPAQUE);
    // Définir la couleur de fond des cellules en bleu clair
    SDL_SetRenderDrawColor(rendu, 200, 200, 255, SDL_ALPHA_OPAQUE);
    
    // Dessiner les lignes verticales et horizontales
    for (int i = 0; i <= TAILLE_GRILLE; ++i) {
        // Ligne verticale
        SDL_RenderDrawLine(rendu, 
                          i * TAILLE_CELLULE, 0, 
                          i * TAILLE_CELLULE, TAILLE_GRILLE * TAILLE_CELLULE);
        // Ligne horizontale
        SDL_RenderDrawLine(rendu, 
                          0, i * TAILLE_CELLULE, 
                          TAILLE_GRILLE * TAILLE_CELLULE, i * TAILLE_CELLULE);
    }
}

// Dessine un bateau sur la grille
void dessiner_bateau(int x, int y, int longueur, int horizontal) {
    SDL_Rect bateau;
    // Calcul de la position et des dimensions du bateau
    bateau.x = x * TAILLE_CELLULE;
    bateau.y = y * TAILLE_CELLULE;
    bateau.w = horizontal ? longueur * TAILLE_CELLULE : TAILLE_CELLULE;
    bateau.h = horizontal ? TAILLE_CELLULE : longueur * TAILLE_CELLULE;

    // Dessiner le bateau en bleu
    SDL_SetRenderDrawColor(rendu, 0, 0, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(rendu, &bateau);
}