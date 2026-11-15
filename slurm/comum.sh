# Funções comuns aos jobs. Incluído com `source` pelos .sbatch.
set -u
cd "$SLURM_SUBMIT_DIR"
source ./config.sh
eval "$AMBIENTE"

# Distribuição dos processos entre os nós: cíclica (rank 0 no nó 1, rank 1 no
# nó 2, rank 2 no nó 1, ...). Na árvore da versão original as folhas são os
# ranks mais altos e consecutivos; com a distribuição em bloco elas cairiam
# quase todas no mesmo nó. Com a cíclica, cada nó recebe metade das folhas.
# --cpu-bind=none deixa o escalonador do Linux escolher o núcleo de cada
# processo (necessário para o balanceamento feito pelo SO nas séries > 32).
SRUN_OPTS="--distribution=cyclic --cpu-bind=none $SRUN_MPI"

cabecalho() {
    echo "==== job $SLURM_JOB_ID ($SLURM_JOB_NAME) em $(date "+%Y-%m-%d %H:%M:%S")"
    echo "nos: $SLURM_JOB_NODELIST   N=$N   REPS=$REPS"
    echo "---- lscpu (nó $(hostname))"
    lscpu
    echo "---- como o Slurm conta as CPUs deste nó (os scripts supõem CPUTot=16)"
    scontrol show node "$(hostname -s)" 2>/dev/null \
        | grep -oE "(CPUTot|ThreadsPerCore|CoresPerSocket|Sockets)=[0-9]+" | tr '\n' ' '
    echo
    echo "---- Open MPI"
    (mpirun --version 2>/dev/null || ompi_info --version 2>/dev/null) | head -2
    ompi_info --param mpi all --level 9 2>/dev/null | grep -i yield || true
    echo "---- variáveis OMPI no ambiente"
    env | grep '^OMPI_' || echo "(nenhuma)"
    echo "===="
}

# Nós usados para P processos (P = 1 cabe em um nó; srun exige N <= P).
nos_para() { if [ "$1" -gt 1 ]; then echo 2; else echo 1; fi; }

# Threads de hardware (CPUs do Slurm) por processo. O Slurm restringe cada
# passo às CPUs pedidas; com 1 CPU por processo, 8 processos num nó poderiam
# ficar em 4 núcleos físicos (2 threads HT cada). Com até 8 processos por nó
# cada um pede 2 CPUs, isto é, um núcleo físico inteiro; acima disso, 1 CPU
# por processo, e o HT passa a ser usado.
cpt_para() {
    local por_no=$(( ($1 + 1) / 2 ))
    if [ "$por_no" -le 8 ]; then echo 2; else echo 1; fi
}

# roda P programa args...: lança P processos com a distribuição acima.
roda() {
    local P=$1; shift
    srun -N "$(nos_para "$P")" -n "$P" -c "$(cpt_para "$P")" $SRUN_OPTS "$@"
}

# Registra o comando e quantos processos ficaram em cada nó com P processos.
mostra_distribuicao() {
    local P=$1
    echo "DISTRIBUICAO P=$P (srun -N $(nos_para "$P") -n $P -c $(cpt_para "$P") $SRUN_OPTS):" \
         "$(roda "$P" hostname | sort | uniq -c | tr '\n' ' ')"
}
