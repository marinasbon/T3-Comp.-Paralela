#!/bin/bash
# Configuração balanceada-isend (extra, não exigida pelo enunciado): a mesma
# série da configuração balanceada, mas com o envio não bloqueante do código
# de Radenski (MPI_Isend + MPI_Request_free, opção -i). Mostra o efeito da
# falta de progresso assíncrono do Open MPI: o ajudante só recebe a metade
# quando o pai volta a chamar o MPI.
#SBATCH --job-name=t3-balanceada-isend
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=16
#SBATCH --exclusive
#SBATCH --time=01:00:00
#SBATCH --output=resultados/%x-%j.out

export OMPI_MCA_mpi_yield_when_idle=0

source slurm/comum.sh
cabecalho

PROCS="2 4 8 16 31 32"
for P in $PROCS; do mostra_distribuicao "$P"; done

for rep in $(seq 1 "$REPS"); do
    echo "---- repeticao $rep"
    for P in $PROCS; do
        roda "$P" ./balanceado "$N" -i
    done
done
