#!/bin/bash
# Configuração seq-arvore-balanceada (seções 5.2 e 6.1): referência T1^delta
# da versão balanceada, a mesma árvore de balanceado.c percorrida em um só
# processo para P = 2, 4, 8, 16, 31 e 32. Com P = 31 as folhas têm tamanhos
# diferentes e nenhum delta único reproduz a árvore; por isso a referência
# recebe P (opção -b), não delta.
#SBATCH --job-name=t3-seq-arvore-balanceada
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --time=00:45:00
#SBATCH --output=resultados/%x-%j.out

source slurm/comum.sh
cabecalho

PROCS="2 4 8 16 31 32"

for rep in $(seq 1 "$REPS"); do
    echo "---- repeticao $rep"
    for P in $PROCS; do
        ./sequencial "$N" -b "$P"
    done
done
