#!/bin/bash
# Configuração balanceada (seção 6.3): versão balanceada (alterando o código)
# com 2, 4, 8, 16, 32 e 31 processos em 2 nós exclusivos. 16: um por núcleo
# físico; 32: usa o HT; 31: compara direto com a original e mostra o que
# acontece quando P não é potência de 2. Envio ao ajudante com MPI_Send.
#SBATCH --job-name=t3-balanceada
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=16
#SBATCH --exclusive
#SBATCH --time=01:00:00
#SBATCH --output=resultados/%x-%j.out

# Espera ativa definida explicitamente, como na configuração original.
export OMPI_MCA_mpi_yield_when_idle=0

source slurm/comum.sh
cabecalho

PROCS="2 4 8 16 31 32"
for P in $PROCS; do mostra_distribuicao "$P"; done

for rep in $(seq 1 "$REPS"); do
    echo "---- repeticao $rep"
    for P in $PROCS; do
        roda "$P" ./balanceado "$N"
    done
done
