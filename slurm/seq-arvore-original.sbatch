#!/bin/bash
# Configuração seq-arvore-original (seções 5.2 e 6.1): referência T1^delta da
# versão original, isto é, a mesma árvore da versão paralela percorrida em um
# só processo, para cada delta = ceil(2N/(P+1)) usado nas séries original e
# sobrecarga (P = 3, 7, ..., 1023). Com P = 1, delta = N e a árvore é o
# próprio T1 (configuração seq-t1).
# As repetições ficam no laço de fora: cada rodada passa por todos os deltas,
# e uma perturbação passageira no nó não cai em um só deles.
#SBATCH --job-name=t3-seq-arvore-original
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --time=00:45:00
#SBATCH --output=resultados/%x-%j.out

source slurm/comum.sh
cabecalho

PROCS="3 7 15 31 63 127 255 511 1023"

for rep in $(seq 1 "$REPS"); do
    echo "---- repeticao $rep"
    for P in $PROCS; do
        ./sequencial "$N" $(( (2 * N + P) / (P + 1) ))      # delta = ceil(2N/(P+1))
    done
done
