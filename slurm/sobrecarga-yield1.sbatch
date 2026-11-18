#!/bin/bash
# Configuração sobrecarga-yield1 (seção 6.4): versão original com 63, 127,
# 255, 511 e 1023 processos nos mesmos 2 nós, mais processos que as 32
# threads de hardware, com OMPI_MCA_mpi_yield_when_idle=1 (o processo
# parado em MPI_Recv cede o núcleo, e o escalonador do Linux pode
# entregá-lo às folhas).
#SBATCH --job-name=t3-sobrecarga-yield1
#SBATCH --nodes=2
#SBATCH --ntasks=1023
#SBATCH --overcommit
#SBATCH --exclusive
#SBATCH --time=03:00:00
#SBATCH --output=resultados/%x-%j.out

export OMPI_MCA_mpi_yield_when_idle=1

source slurm/comum.sh
cabecalho

PROCS="63 127 255 511 1023"
SRUN_OPTS="$SRUN_OPTS --overcommit"   # mais processos que CPUs
for P in $PROCS; do mostra_distribuicao "$P"; done

for rep in $(seq 1 "$REPS"); do
    echo "---- repeticao $rep"
    for P in $PROCS; do
        DELTA=$(( (2 * N + P) / (P + 1) ))                 # ceil(2N/(P+1))
        roda "$P" ./paralelo "$N" "$DELTA"
    done
done
