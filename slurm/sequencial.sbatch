#!/bin/bash
# Referências sequenciais, em um nó (seções 5.2 e 6.1):
#   T1                Bubble Sort do vetor inteiro
#   T1^delta          árvore da versão original para cada delta usado
#                     (delta = ceil(2N/(P+1)), P = 3, 7, ..., 1023)
#   T1^delta (bal.)   árvore da versão balanceada para P = 2, 4, 8, 16, 31, 32
# As repetições ficam no laço de fora: cada rodada passa por todas as
# configurações, e uma perturbação passageira no nó não cai em uma só delas.
#SBATCH --job-name=t3-sequencial
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --time=01:30:00
#SBATCH --output=resultados/%x-%j.out

source slurm/comum.sh
cabecalho

for rep in $(seq 1 "$REPS"); do
    echo "---- repeticao $rep"
    ./sequencial "$N"                                       # T1
    for P in 3 7 15 31 63 127 255 511 1023; do
        ./sequencial "$N" $(( (2 * N + P) / (P + 1) ))      # T1^delta, original
    done
    for P in 2 4 8 16 31 32; do
        ./sequencial "$N" -b "$P"                           # T1^delta, balanceada
    done
done
