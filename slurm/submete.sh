#!/bin/bash
# Compila e submete todos os jobs, sempre nos mesmos nós (config.sh: NOS).
# Uso, a partir da raiz do repositório:
#   ./slurm/submete.sh calibra     só a calibração do tamanho do vetor
#   ./slurm/submete.sh             todas as medições
set -eu
cd "$(dirname "$0")/.."
source ./config.sh
eval "$AMBIENTE"

make
mkdir -p resultados

PART=""
[ -n "$PARTICAO" ] && PART="--partition=$PARTICAO"
NO1=${NOS%%,*}

if [ "${1:-}" = "calibra" ]; then
    sbatch $PART -w "$NO1" slurm/calibra.sbatch
    exit 0
fi

sbatch $PART -w "$NO1" slurm/sequencial.sbatch
sbatch $PART -w "$NOS" slurm/original.sbatch
sbatch $PART -w "$NOS" slurm/balanceado.sbatch
sbatch $PART -w "$NOS" --export=ALL,YIELD=0 slurm/sobrecarga.sbatch
sbatch $PART -w "$NOS" --export=ALL,YIELD=1 slurm/sobrecarga.sbatch
squeue -u "$USER"
