# T3: divisão e conquista com MPI

Ordenação de um vetor (Bubble Sort nas folhas, intercalação na volta) com MPI,
na versão original (árvore binária completa, Figura 1 do enunciado) e na versão
balanceada (adaptada de Radenski, `mpi_mergesort.c`, GPL).

## Arquivos

| Caminho | O que é |
|---|---|
| `src/ordenacao.h` | `bubble_sort` e `intercala` do `sequencial.c` distribuído (sem alteração), inicialização e verificação |
| `src/sequencial.c` | referências: `./sequencial n` (T1), `./sequencial n delta` (T1^δ da original), `./sequencial n -b P` (T1^δ da balanceada) |
| `src/paralelo.c` | versão original: `srun -n P ./paralelo n [delta]` (δ omitido = ⌈2n/(P+1)⌉) |
| `src/balanceado.c` | versão balanceada: `srun -n P ./balanceado n [-i]` (`-i` = `MPI_Isend` + `MPI_Request_free` como Radenski) |
| `config.sh` | **editar antes de tudo**: N, nós, partição, ambiente |
| `MEDICOES.md` | configuração de cada medição exigida (P, δ, nós, distribuição, `yield`) e o script de cada uma |
| `slurm/<configuração>.sbatch` | um job por configuração: `calibra`, `seq-t1`, `seq-arvore-original`, `seq-arvore-balanceada`, `original`, `balanceada`, `sobrecarga-yield0`, `sobrecarga-yield1` e `balanceada-isend` (extra) |
| `slurm/submete.sh` | compila e submete todas as configurações (ou só as indicadas pelo nome) nos mesmos nós |
| `analise/analisa.py` | lê `resultados/*.out` e calcula tempos (mediana, mín., máx.), S, S^δ e E, com gráficos, em `resultados/analise/` |

Cada execução imprime uma linha `RESULTADO` (`versao`, `P`, `n`, `delta`,
`yield`, folhas, `tempo`, `verificacao`); a original imprime também uma linha
`NIVEL` por nível da árvore, com o tempo médio de cada fase.

## Passo a passo no Atlântica

1. Clonar o repositório no cluster.
2. Editar `config.sh`: `NOS` (dois nós de `sinfo -N`), `PARTICAO`, `AMBIENTE`
   (ex.: `module load openmpi`) e `SRUN_MPI` se o `srun` precisar de `--mpi=pmix`.
3. Calibrar o tamanho do vetor:
   `./slurm/submete.sh calibra` e ver `resultados/t3-calibra-*.out`.
   Escolher N com T1 entre 30 e 60 s (o tempo cresce com N²) e gravar em `config.sh`.
4. Submeter tudo: `./slurm/submete.sh` (ou só algumas configurações:
   `./slurm/submete.sh original seq-t1`; ver `MEDICOES.md`). Confira em cada `.out`:
   o `lscpu`, as linhas `DISTRIBUICAO` (processos por nó) e `verificacao=OK`.
5. `make analise` (precisa de Python 3 com matplotlib): gera
   `resultados/analise/resumo.md`, `speedup.png` e `sobrecarga.png`.
   Avisos sobre repetições faltando aparecem no terminal.

## Se algo der errado no cluster

- **`sbatch` recusa `--ntasks-per-node=16`, ou o `srun -c 2` falha**: os scripts
  supõem que o Slurm conta cada thread de HT como uma CPU (`CPUTot=16`, impresso
  no início de todo `.out`, inclusive o da calibração). Se aparecer `CPUTot=8`,
  troque `--ntasks-per-node=16` por `--ntasks-per-node=8 --overcommit` nos
  `.sbatch` e, em `slurm/comum.sh`, faça `cpt_para` devolver sempre 1.
- **`srun` não acha o MPI** (erro de PMI): defina `SRUN_MPI="--mpi=pmix"` (ou
  `pmi2`) em `config.sh`; `srun --mpi=list` mostra as opções.
- **Job some sem saída**: confira se a pasta `resultados/` existe (o
  `submete.sh` a cria) e o `--time` do job.

## Decisões de implementação

- **Distribuição cíclica** (`--distribution=cyclic`): as folhas da original são os
  ranks mais altos e consecutivos; em bloco, ficariam quase todas no mesmo nó.
- **`-c 2` com até 8 processos por nó**: o Slurm restringe cada passo às CPUs
  pedidas; com 1 CPU (= 1 thread) por processo, 8 processos poderiam ficar em 4
  núcleos físicos. Com 31/32 processos, `-c 1` e o HT é usado.
- **`OMPI_MCA_mpi_yield_when_idle=0` explícito** nas séries de 1 a 32
  processos, pela errata do enunciado; a série de 63 a 1023 roda com 0 e com 1.
- **Barreira antes de `t0`**: o lançamento dos processos fica fora da região medida.
- **Original com 2L−1 processos ≡ balanceada com L** (L potência de 2): mesmas
  folhas, mesmo T1^δ. Ex.: original-63 e balanceada-32.
- **`MPI_Send` em vez de `MPI_Isend` na balanceada**: com `MPI_Isend` +
  `MPI_Request_free`, o ajudante só recebe a metade quando o pai volta ao MPI
  (mensagem grande = protocolo rendezvous, sem progresso assíncrono no Open MPI),
  e a árvore roda em série (~2,7× mais lenta com 4 processos no teste local).
  A série `balanceada_isend` mede isso no cluster.
- **Balanceada com 31**: o rank 15 não tem ajudante no nível 4 (seria 31) e ordena
  n/16, 4× o trabalho das demais folhas: T31 ≈ T16.
