#!/bin/bash

# Compilation
echo "Compilation du programme..."
gcc -o main main.c
if [ $? -ne 0 ]; then
    echo "Erreur à la compilation !"
    exit 1
fi
echo "Compilation OK."

# Création du dossier sortie
mkdir -p sortie

# En-tête du CSV
echo "Essai, Livrees, Entrepot, Prod, AVG1, AVG2, Cmds, Cmds_non_opt, Charge_moy, Temps_fin" > resultats.csv

# Boucle d'exécution
for i in $(seq 1 100)
do
    echo "Exécution $i..."
    sortie=$(./main)
    code_retour=$?

    # Sauvegarde la sortie brute dans le dossier sortie
    echo "$sortie" > sortie/sortie_${i}.txt

    if [ $code_retour -ne 0 ]; then
        echo "Erreur lors de l'exécution $i (code $code_retour)"
        echo "$i,ERREUR" >> resultats.csv
        continue
    fi

    # Extraction des données
    livrees=$(echo "$sortie" | grep "Nombre de pieces livrees" | awk -F':' '{print $2}' | tr -d ' ')
    entrepot=$(echo "$sortie" | grep "Nombre de pieces a l.entrepot" | awk -F':' '{print $2}' | tr -d ' ')
    prod=$(echo "$sortie" | grep "Nombre de pieces a la prod" | awk -F':' '{print $2}' | tr -d ' ')
    avg1=$(echo "$sortie" | grep "Nombre de pieces dans AVG1" | awk -F':' '{print $2}' | tr -d ' ')
    avg2=$(echo "$sortie" | grep "Nombre de pieces dans AVG2" | awk -F':' '{print $2}' | tr -d ' ')
    cmds=$(echo "$sortie" | grep "Nombre de commandes :" | awk -F':' '{print $2}' | tr -d ' ')
    cmds_non_opt=$(echo "$sortie" | grep "Nombre de commandes non optimisées" | awk -F':' '{print $2}' | tr -d ' ')
    charge=$(echo "$sortie" | grep "Charge moyenne par AGV" | awk -F':' '{print $2}' | tr -d ' ')
    temps=$(echo "$sortie" | grep "Simulation terminee au temps t" | awk -F'=' '{print $2}' | tr -d ' ')

    # Affichage pour suivi
    echo "  Livrees=$livrees, Entrepot=$entrepot, Prod=$prod, AVG1=$avg1, AVG2=$avg2, Cmds=$cmds, Cmds_non_opt=$cmds_non_opt, Charge_moy=$charge, Temps_fin=$temps"

    # Ajout au CSV
    echo "$i,$livrees,$entrepot,$prod,$avg1,$avg2,$cmds,$cmds_non_opt,$charge,$temps" >> resultats.csv
done

echo "Terminé ! Les résultats sont dans resultats.csv, et les sorties brutes dans le dossier 'sortie/'"
