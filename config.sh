# Configuração comum a todos os jobs (lida pelos scripts em slurm/).

# Tamanho do vetor. Escolher com slurm/calibra.sbatch para que T1 (Bubble Sort
# do vetor inteiro, ./sequencial N) fique entre 30 e 60 s no Atlântica.
N=250000

# Repetições por configuração (seção 5.5 do enunciado).
REPS=10

# Os mesmos 2 nós em todos os jobs (seção 5.1). Preencher com os nomes vistos
# em `sinfo -N`; a referência sequencial roda no primeiro deles.
NOS="NO1,NO2"

# Partição do Slurm (deixe vazio para usar a padrão).
PARTICAO=""

# Opções extras do srun para lançar o Open MPI (ex.: "--mpi=pmix"), se o
# cluster exigir. Deixe vazio se o srun já funcionar sem elas.
SRUN_MPI=""

# Comandos para carregar o ambiente (ex.: "module load openmpi"), se houver.
AMBIENTE=""
