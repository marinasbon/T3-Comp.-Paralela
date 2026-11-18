#!/bin/bash
# Configuração seq-t1 (seções 5.2 e 6.1): referência T1, o Bubble Sort do
# vetor inteiro em um só processo, em um nó do Atlântica.
#SBATCH --job-name=t3-seq-t1
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --time=00:30:00
#SBATCH --output=resultados/%x-%j.out

source slurm/comum.sh
cabecalho

for rep in $(seq 1 "$REPS"); do
    echo "---- repeticao $rep"
    ./sequencial "$N"
done
