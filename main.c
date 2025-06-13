#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_EVENTS 3

long z0;           // Variable pour le generateur de nombres aleatoires
float H = 36000;   // horizon de simulation
float t = 0;       // temps actuel
int warehouse = 0; // 0 : disponible, 1 : occupe
int stock_agv1 = 0, stock_agv2 = 0; // nombre de pieces dans les robots
int agv1 = 0, agv2 = 1;             // 0 : pas en attente, 1 : en attente
int indiceCommandesAVG1 =
    0; // permet de savoir a quelle commande on en est pour AGV1
int indiceCommandeAVG2 =
    0; // permet de savoir a quelle commande on en est pour AGV2

// A MODIFIER POUR CHANGER LES COMMANDES
int nb_tot_pieces = 64;      // nombre total de pieces a livrer
int F1 = 64, F2 = 0, F3 = 0; // stocks ; F1 = nb_tot_pieces
int TabCommande[64];         // tableau de commandes

int nbCommandes;
int nbCommandesNonOptimisees;

float tab[2][MAX_EVENTS]; // echeancier
int event_count = 0;      // nombre d'evenements dans l'echeancier

// Liste des procédures pour éviter les erreurs
// 1 : AGV1 arrive a la production
void agv1_arrivee_prod();
// 2 : AGV1 chargement termine
void agv1_chargement_termine();
// 3 : AGV1 arrive a l'entrepot
void agv1_arrivee_warehouse();
// 4 : AGV1 dechargement termine
void agv1_dechargement_termine();
// 5 : AGV2 arrive a l'entrepot
void agv2_arrivee_warehouse();
// 6 : AGV2 chargement termine
void agv2_chargement_termine();
// 7 : AGV2 arrive au client
void agv2_arrivee_client();
// 8 : AGV2 dechargement termine
void agv2_dechargement_termine();

// Fonction pour generer un nombre aleatoire uniforme entre 0 et 1
double U01() {
  long a = 6543;
  long b = 534;
  long c = 43;
  z0 = (a * z0 + b) % c;
  return z0 / (double)c;
}

// Fonction pour generer un nombre aleatoire suivant une loi normale
float normale(float m, float n) {
  float U1, U2, W, X, Y;
  U1 = U01();
  U2 = U01();
  W = sqrt(-2.0 * log(U1));
  X = W * cos(2.0 * M_PI * U2);
  return m + X * n;
}

// Fonction pour ajouter un evenement a l'echeancier
void ajouter(float date, int type) {
  if (event_count < MAX_EVENTS) {
    int i = 0;
    while (i < event_count && tab[0][i] < date) {
      i++;
    }

    for (int j = event_count; j > i; j--) {
      tab[0][j] = tab[0][j - 1];
      tab[1][j] = tab[1][j - 1];
    }

    tab[0][i] = date;
    tab[1][i] = type;
    event_count++;
  }
}

// Fonction pour supprimer un evenement de l'echeancier ou de q
void supprimer(int tableau) {
  if (tableau == 1 && event_count > 0) {
    // Supprimer dans tab
    for (int i = 0; i < event_count - 1; i++) {
      tab[0][i] = tab[0][i + 1];
      tab[1][i] = tab[1][i + 1];
    }
    tab[0][event_count - 1] = 2 * H; // Marquer la fin de l'echeancier
    tab[1][event_count - 1] = 0;     // Marquer la fin de l'echeancier
    event_count--;
  }
}

void OptiCommandes() {
  int idTab = 0;
  int commandesSansTri[64];
  for (int i = 0; i < nbCommandes; i++) {
    commandesSansTri[i] = TabCommande[i];
  }
  for (int i = 0; i < nbCommandes; i++) {
    if (commandesSansTri[i] != 0) {

      int tt = commandesSansTri[i];
      int max = tt;
      int idmax = -1;
      for (int j = i + 1; j < nbCommandes; j++) {
        if (tt + commandesSansTri[j] > max && tt + commandesSansTri[j] <= 6) {
          max = tt + commandesSansTri[j];
          idmax = j;
        }
      }
      if (max > tt) {
        commandesSansTri[idmax] = 0;
      }
      TabCommande[idTab] = max;
      idTab += 1;
    }
  }
  nbCommandes = idTab;
  printf("[prepCommandes] Résumé des commandes optimisées (%d total) : ",
         nbCommandes);
  for (int j = 0; j < nbCommandes; j++) {
    printf("%d ", TabCommande[j]);
  }
  printf("\n");
}

void preparationCommandes() {
  int somme = 0;
  int a;
  int i = 0;
  while (somme < nb_tot_pieces) {
    a = floor(6 * U01() + 1); // Générer un nombre aléatoire entre 1 et 7
    TabCommande[i] = a;
    somme += a;
    i += 1;
    if (somme > 64) {
      somme -= a;
      i -= 1;
      TabCommande[i] = 0;
    }
  }
  nbCommandes = i;
  nbCommandesNonOptimisees = i;

  printf("[prepCommandes] Résumé des commandes (%d total) : ", nbCommandes);
  for (int j = 0; j < nbCommandes; j++) {
    printf("%d ", TabCommande[j]);
  }
  printf("\n");

  OptiCommandes();
}

// Fonctions pour les evenements
void agv1_arrivee_prod() {
  printf("AGV1 arrive a la production (t=%.2f)\n", t);
  if (F1 > 0) {
    stock_agv1 =
        TabCommande[indiceCommandesAVG1]; // Prendre la commande suivante
    F1 -= stock_agv1;      // Retirer les pieces du stock de production
    indiceCommandesAVG1++; // Passer a la commande suivante
    printf("AGV1 charge %d pieces (t=%.2f)\n", stock_agv1, t);
    ajouter(t + normale(stock_agv1 * 17.82, 1.0),
            2); // simuler duree de chargement
  } else if (F1 == 0) {
    printf("F1 Vide)\n");
  }
}

void agv1_chargement_termine() {
  printf("AGV1 chargement termine (t=%.2f)\n", t);
  printf("AGV1 deplace %d pieces vers l'entrepot (t=%.2f)\n", stock_agv1, t);
  ajouter(t + normale(28.83, 1.0),
          3); // simuler temps de parcours (moyenne entre 29.72 et 27.933)
}

void agv1_arrivee_warehouse() {
  if (warehouse == 0) {
    printf("AGV1 arrive a l'entrepot & decharge ses pieces (t=%.2f)\n", t);
    warehouse = 1;
    ajouter(t + normale(stock_agv1 * 18.275, 0.5),
            4); // simuler duree de depot (moyenne entre 18.26 et 18.29)
    F2 += stock_agv1;
    stock_agv1 = 0;
  } else {
    agv1 = 1; // agv1 en attente
    printf("AGV1 en attente a l'entrepot (t=%.2f)\n", t);
  }
}

void agv1_dechargement_termine() {
  printf("AGV1 dechargement termine (t=%.2f)\n", t);
  warehouse = 0;
  if (agv2 == 1) {
    agv2 = 0;
    agv2_arrivee_warehouse();
  }
  ajouter(t + normale(24.442, 1.0),
          1); // simuler date arrivee prod (moyenne de 24.442)
}

void agv2_arrivee_warehouse() {
  printf("AGV2 arrive a l'entrepot (t=%.2f)\n", t);
  if (warehouse == 0 && F2 >= 0) {
    warehouse = 1;
    printf("AGV2 preleve %d pieces de l'entrepot (t=%.2f)\n",
           TabCommande[indiceCommandeAVG2], t);
    F2 -= TabCommande[indiceCommandeAVG2]; // Retirer les pieces du stock de
                                           // l'entrepot
    stock_agv2 = TabCommande[indiceCommandeAVG2];
    indiceCommandeAVG2++; // Passer a la commande suivante
    ajouter(t + normale(stock_agv2 * 31.0685, 1.0),
            6); // simuler prelevement (moyenne entre 29.386 et 32.751)
  } else {
    agv2 = 1; // agv2 en attente
    printf("AGV2 en attente a l'entrepot (t=%.2f)\n", t);
  }
}

void agv2_chargement_termine() {
  printf("AGV2 chargement termine (t=%.2f)\n", t);
  warehouse = 0;
  ajouter(t + normale(30.653, 1.0),
          7); // simuler deplacement (moyenne de 30.653)
  if (agv1 == 1) {
    agv1 = 0;
    agv1_arrivee_warehouse();
  }
}

void agv2_arrivee_client() {
  printf("AGV2 arrive au client (t=%.2f)\n", t);
  printf("AGV2 livre %d pieces au client (t=%.2f)\n", stock_agv2, t);
  ajouter(t + normale(stock_agv2 * 18.275, 0.5),
          8); // simuler temps de dechargement (moyenne entre 18.26 et 18.29)
}

void agv2_dechargement_termine() {
  printf("AGV2 dechargement termine (t=%.2f)\n", t);

  F3 += stock_agv2; // Ajouter les pieces livrees a F3
  printf("AGV2 a livre %d pieces (t=%.2f)\n", stock_agv2, t);
  stock_agv2 = 0;
  if (F3 == nb_tot_pieces) {
    printf("Toutes les pieces ont ete livrees au bout de %.2fs\n", t);
  } else {
    ajouter(t + normale(31.0685, 1.0),
            5); // simuler deplacement (moyenne entre 29.386 et 32.751)
  }
}

int main() {
  srand(time(NULL)); // Initialisation du generateur de nombres aleatoires

  // Initialiser z0 avec le temps actuel
  z0 = time(NULL);

  preparationCommandes(); // Initialisation des commandes

  agv1_arrivee_prod(); // Initialisation du premier evenement
  double chargeMoyenneAVG = 0.0;

  while (t < H) {
    if (event_count >= MAX_EVENTS) {
      printf("[ERREUR] Echéancier plein !\n");
      exit(EXIT_FAILURE);
    }
    t = tab[0][0];
    int type = (int)tab[1][0];
    supprimer(1); // Supprimer dans tab

    switch (type) {
    case 1:
      agv1_arrivee_prod();
      break;
    case 2:
      agv1_chargement_termine();
      break;
    case 3:
      agv1_arrivee_warehouse();
      break;
    case 4:
      agv1_dechargement_termine();
      break;
    case 5:
      agv2_arrivee_warehouse();
      break;
    case 6:
      agv2_chargement_termine();
      break;
    case 7:
      agv2_arrivee_client();
      break;
    case 8:
      agv2_dechargement_termine();
      break;
    }
  }

  // Affichage des resultats finaux
  printf("\n_____________________________________\n");
  printf("\nRESULTATS FINAUX\n");
  printf("_____________________________________\n");
  printf("Nombre de pieces livrees (F3) : %d\n", F3);
  printf("Nombre de pieces a l'entrepot (F2) : %d\n", F2);
  printf("Nombre de pieces a la prod (F1) : %d\n", F1);
  printf("_____________________________________\n");
  printf("Nombre de pieces dans AVG1 : %d\n", stock_agv1);
  printf("Nombre de pieces dans AVG2 : %d\n", stock_agv2);
  printf("Nombre de commandes : %d\n", nbCommandes);
  printf("Nombre de commandes non optimisées : %d\n", nbCommandesNonOptimisees);
  
  for (int j = 0; j < nbCommandes; j++) {
    chargeMoyenneAVG += TabCommande[j];
  }
  chargeMoyenneAVG /= nbCommandes;
  
  printf("Charge moyenne par AGV : %f\n", chargeMoyenneAVG);
  printf("_____________________________________\n");
  printf("Simulation terminee au temps t=%.2f\n", t);
  printf("_____________________________________\n");
  printf("Fin de la simulation.\n");

  return 0;
}
