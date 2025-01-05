# README.md

## Projet : Jeu de la Bataille Navale avec SDL

Ce projet implémente un jeu de la bataille navale en C avec une interface graphique basée sur la bibliothèque SDL2. Voici les étapes pour configurer et lancer l'environnement sur un système Linux.

---

### Prérequis

Assurez-vous que les outils suivants sont installés sur votre système :

- **Compilateur GCC** : pour compiler le code C.
- **Make** : pour automatiser la compilation.
- **SDL2** : bibliothèque pour l'interface graphique.

Pour les installer, utilisez les commandes suivantes (sur une distribution basée sur Debian, comme Ubuntu) :

```bash
sudo apt update
sudo apt install gcc make libsdl2-dev
```

---

### Structure du Projet

Voici la structure des fichiers du projet :

```
|-- src/
|   |-- main.c          # Fichier principal du jeu
|   |-- client.c        # Logique du client
|   |-- server.c        # Logique du serveur
|   |-- game_logic.c    # Gestion des règles du jeu
|-- include/
|   |-- game_logic.h    # Déclaration des fonctions de logique de jeu
|-- assets/             # Ressources graphiques (images, icônes, etc.)
|-- Makefile            # Fichier de build pour compiler le projet
|-- README.md           # Instructions pour configurer et lancer le projet
```

---

### Compilation

1. Clonez le dépôt du projet :
   ```bash
   git clone <url-du-repository>
   cd <nom-du-repertoire>
   ```

2. Compilez le projet avec `make` :
   ```bash
   make
   ```
   Cela génère un exécutable appelé `battleship`.

---

### Lancer le Serveur et le Client

#### Étape 1 : Lancer le serveur

Dans un terminal, exécutez :
```bash
./battleship server
```

#### Étape 2 : Lancer le client

Dans un autre terminal, exécutez :
```bash
./battleship client
```

---

### Développement et Tests

- **Ajout de fonctionnalités** : Le code source se trouve dans le dossier `src/`. Ajoutez vos fonctions ou fichiers dans ce répertoire et mettez à jour le `Makefile` si nécessaire.
- **Ressources graphiques** : Placez vos images ou fichiers graphiques dans le répertoire `assets/`.
- **Tests** : Les tests manuels peuvent être effectués en lançant plusieurs instances du client et en interagissant avec le serveur.

---

### Dépannage

- **Erreur de SDL** : Assurez-vous que `libsdl2-dev` est bien installé. Vérifiez avec :
  ```bash
  sdl2-config --version
  ```
- **Erreur de compilation** : Vérifiez que toutes les dépendances sont présentes et utilisez `make clean` avant de recompiler.
  ```bash
  make clean
  make
  ```

---

### Remerciements

Ce projet utilise [SDL2](https://www.libsdl.org/) pour l'affichage graphique et est développé dans le cadre d'un apprentissage sur les réseaux et le développement de jeux.

Ce projet a été réalisé par Kristen ROPARS et Lohan FERNANDEZ MARTINEZ, dans le cadre du module Réseau de la formation en Master 1 MIAGE.
