#!/bin/bash
# Compila e submete os jobs de medição, sempre nos mesmos nós (config.sh: NOS).
# Cada configuração descrita em MEDICOES.md tem um script slurm/<nome>.sbatch.
# Uso, a partir da raiz do repositório:
#   ./slurm/submete.sh calibra                só a calibração do tamanho do vetor
#   ./slurm/submete.sh                        todas as medições (sem a calibração)
#   ./slurm/submete.sh original seq-t1 ...    só as configurações indicadas
set -eu
cd "$(dirname "$0")/.."
source ./config.sh
eval "$AMBIENTE"

# As referências sequenciais (e a calibração) rodam em um nó, o primeiro de
# NOS; as versões paralelas, nos dois.
SEQUENCIAIS="seq-t1 seq-arvore-original seq-arvore-balanceada"
PARALELAS="original balanceada balanceada-isend sobrecarga-yield0 sobrecarga-yield1"

[ $# -eq 0 ] && set -- $SEQUENCIAIS $PARALELAS

# Confere todos os nomes antes de submeter qualquer job.
for nome in "$@"; do
    [ -f "slurm/$nome.sbatch" ] || { echo "configuração desconhecida: $nome" >&2; exit 1; }
done

make
mkdir -p resultados

PART=""
[ -n "$PARTICAO" ] && PART="--partition=$PARTICAO"
NO1=${NOS%%,*}

for nome in "$@"; do
    case " calibra $SEQUENCIAIS " in
        *" $nome "*) nos="$NO1" ;;
        *)           nos="$NOS" ;;
    esac
    sbatch $PART -w "$nos" "slurm/$nome.sbatch"
done
squeue -u "$USER"
