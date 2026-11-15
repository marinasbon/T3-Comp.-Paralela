#!/bin/bash
# Versão balanceada com 2, 4, 8, 16, 32 e 31 processos em 2 nós exclusivos
# (seção 6.3). 16: um por núcleo físico; 32: usa o HT; 31: P não é potência de 2.
# Série extra: a mesma versão com o envio não bloqueante do código de Radenski
# (opção -i), para mostrar o efeito do progresso do MPI_Isend.
#SBATCH --job-name=t3-balanceado
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

PROCS="2 4 8 16 31 32"
for P in $PROCS; do mostra_distribuicao "$P"; done

for rep in $(seq 1 "$REPS"); do
    echo "---- repeticao $rep"
    for P in $PROCS; do
        roda "$P" ./balanceado "$N"            # envio com MPI_Send (padrão)
        roda "$P" ./balanceado "$N" -i         # MPI_Isend + MPI_Request_free (Radenski)
    done
done
