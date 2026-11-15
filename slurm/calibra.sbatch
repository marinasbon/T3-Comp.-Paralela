#!/bin/bash
# Calibração do tamanho do vetor: T1 deve ficar entre 30 e 60 s (seção 5.2).
# O Bubble Sort é quadrático: se n leva t segundos, n' leva t * (n'/n)^2.
#SBATCH --job-name=t3-calibra
#SBATCH --nodes=1
#SBATCH --exclusive
#SBATCH --time=00:30:00
#SBATCH --output=resultados/%x-%j.out

source slurm/comum.sh
cabecalho
for n in 150000 200000 250000 300000; do
    ./sequencial "$n"
done
