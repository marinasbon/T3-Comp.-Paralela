#!/bin/bash
# Versão original com 1, 3, 7, 15 e 31 processos em 2 nós exclusivos (seção 6.2).
# 15 processos: um por núcleo físico (16 núcleos nos 2 nós); 31: usa o HT.
#SBATCH --job-name=t3-original
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=16
#SBATCH --exclusive
#SBATCH --time=01:00:00
#SBATCH --output=resultados/%x-%j.out

# Espera ativa definida explicitamente (errata do enunciado: sem a variável,
# o comportamento depende de como o programa é lançado). Mesmo valor da série
# de 63 a 1023 processos com yield=0, para as séries serem comparáveis.
export OMPI_MCA_mpi_yield_when_idle=0

source slurm/comum.sh
cabecalho

PROCS="1 3 7 15 31"
for P in $PROCS; do mostra_distribuicao "$P"; done

for rep in $(seq 1 "$REPS"); do
    echo "---- repeticao $rep"
    for P in $PROCS; do
        DELTA=$(( (2 * N + P) / (P + 1) ))                  # ceil(2N/(P+1))
        roda "$P" ./paralelo "$N" "$DELTA"
    done
done
