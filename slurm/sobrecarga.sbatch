#!/bin/bash
# Versão original com 63, 127, 255, 511 e 1023 processos nos mesmos 2 nós
# (seção 6.4): mais processos que as 32 threads de hardware.
# Submeter duas vezes, com a variável definida explicitamente:
#   sbatch --export=ALL,YIELD=0 slurm/sobrecarga.sbatch   (espera ativa)
#   sbatch --export=ALL,YIELD=1 slurm/sobrecarga.sbatch   (cede o núcleo)
#SBATCH --job-name=t3-sobrecarga
#SBATCH --nodes=2
#SBATCH --ntasks=1023
#SBATCH --overcommit
#SBATCH --exclusive
#SBATCH --time=03:00:00
#SBATCH --output=resultados/%x-%j.out

: "${YIELD:?defina YIELD=0 ou YIELD=1 com --export}"
export OMPI_MCA_mpi_yield_when_idle=$YIELD

source slurm/comum.sh
cabecalho

PROCS="63 127 255 511 1023"
SRUN_OPTS="$SRUN_OPTS --overcommit"   # mais processos que CPUs
for P in $PROCS; do mostra_distribuicao "$P"; done

for rep in $(seq 1 "$REPS"); do
    echo "---- repeticao $rep"
    for P in $PROCS; do
        DELTA=$(( (2 * N + P) / (P + 1) ))
        roda "$P" ./paralelo "$N" "$DELTA"
    done
done
