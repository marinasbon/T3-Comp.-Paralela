/*
 * T3: divisão e conquista com MPI, versão BALANCEADA.
 *
 * Adaptada de mpi_mergesort.c, de A. Radenski (licença GPL), descrito em
 * "Shared Memory, Message Passing, and Hybrid Merge Sorts for Standalone and
 * Clustered SMPs", PDPTA'11, Fig. 4 e 5. Mudanças em relação ao original:
 *   - as folhas ordenam com o Bubble Sort (lá: merge sort sequencial) e o
 *     vetor começa em ordem decrescente (lá: aleatório), como na versão
 *     original deste trabalho;
 *   - o tempo é medido com MPI_Wtime() na mesma região de paralelo.c;
 *   - o pai do ajudante é calculado a partir do rank, em vez de vir de
 *     MPI_ANY_SOURCE;
 *   - o envio ao ajudante é bloqueante (MPI_Send) por padrão; o
 *     MPI_Isend + MPI_Request_free do original fica na opção -i.
 * Este arquivo é distribuído sob a mesma licença GPL.
 *
 * Ideia: ao dividir, o processo envia a metade direita a um ajudante e FICA
 * com a metade esquerda, que continua dividindo no nível seguinte. Assim os
 * processos internos também conquistam. No nível l, o ajudante do processo r
 * é r + 2^l: no nível 0 a raiz chama o 1; no nível 1, 0 chama 2 e 1 chama 3;
 * no nível 2, 0..3 chamam 4..7; e assim por diante. Cada nível dobra o número
 * de processos ocupados, e os ranks novos de cada nível são exatamente
 * [2^l, 2^(l+1)). Um processo de rank h entra na árvore no nível
 * L = nivel_topo(h) (o menor L com 2^L > h) e seu pai é h - 2^(L-1).
 *
 * uso: srun -n P ./balanceado n [-i]
 *   n   tamanho do vetor; a profundidade da árvore sai de P (não há delta)
 *   -i  envia ao ajudante com MPI_Isend + MPI_Request_free, como Radenski
 *       (padrão: MPI_Send, porque sem progresso assíncrono no Open MPI o
 *       ajudante só recebe a metade quando o pai volta a chamar o MPI, e a
 *       árvore roda em série)
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ordenacao.h"

#define TAG_DADOS 1
#define TAG_RESULTADO 2

static int meu_rank, P;
static int envio_radenski = 0;

static int folha = 0; /* tamanho do pedaço que este processo ordenou */

static int nivel_topo(int rank)
{
    int nivel = 0;
    while ((1 << nivel) <= rank)
        nivel++;
    return nivel;
}

static void ordena_paralelo(int *v, int tam, int *aux, int nivel)
{
    int ajudante = meu_rank + (1 << nivel);

    if (ajudante >= P)
    {
        bubble_sort(v, tam);
        folha = tam;
        return;
    }

    int meio = tam / 2;

    if (envio_radenski)
    {
        MPI_Request pedido;
        MPI_Isend(v + meio, tam - meio, MPI_INT, ajudante, TAG_DADOS,
                  MPI_COMM_WORLD, &pedido);
        MPI_Request_free(&pedido);
    }
    else
    {
        MPI_Send(v + meio, tam - meio, MPI_INT, ajudante, TAG_DADOS,
                 MPI_COMM_WORLD);
    }

    ordena_paralelo(v, meio, aux, nivel + 1);

    MPI_Recv(v + meio, tam - meio, MPI_INT, ajudante, TAG_RESULTADO,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    intercala(v, tam, aux);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &meu_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &P);

    int n = 0, posicionais = 0;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-i") == 0)
            envio_radenski = 1;
        else
        {
            n = atoi(argv[i]);
            posicionais++;
        }
    }
    if (n < 1 || posicionais != 1)
    {
        if (meu_rank == 0)
            fprintf(stderr, "uso: %s n [-i]\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    int nivel = nivel_topo(meu_rank);

    int *vetor = NULL, *aux = NULL;
    int tam_vetor = 0;
    if (meu_rank == 0)
    {
        tam_vetor = n;
        vetor = malloc((size_t)n * sizeof(int));
        aux = malloc((size_t)n * sizeof(int));
        inicializa(vetor, n);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    int pai = -1;
    if (meu_rank != 0)
    {
        pai = meu_rank - (1 << (nivel - 1));
        MPI_Status status;
        MPI_Probe(pai, TAG_DADOS, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_INT, &tam_vetor);
        vetor = malloc((size_t)tam_vetor * sizeof(int));
        aux = malloc((size_t)tam_vetor * sizeof(int));
        MPI_Recv(vetor, tam_vetor, MPI_INT, pai, TAG_DADOS,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    ordena_paralelo(vetor, tam_vetor, aux, nivel);

    double tempo = 0.0;
    if (meu_rank != 0)
        MPI_Send(vetor, tam_vetor, MPI_INT, pai, TAG_RESULTADO, MPI_COMM_WORLD);
    else
        tempo = MPI_Wtime() - t0;

    /* ---- fora da região medida: verificação e relatório ---- */
    int *folhas = NULL;
    if (meu_rank == 0)
        folhas = malloc((size_t)P * sizeof(int));
    MPI_Gather(&folha, 1, MPI_INT, folhas, 1, MPI_INT, 0, MPI_COMM_WORLD);

    long long erros = 0;
    if (meu_rank == 0)
    {
        erros = verifica(vetor, n);
        const char *yield = getenv("OMPI_MCA_mpi_yield_when_idle");
        int menor = n, maior = 0;
        for (int r = 0; r < P; r++)
        {
            if (folhas[r] < menor)
                menor = folhas[r];
            if (folhas[r] > maior)
                maior = folhas[r];
        }
        printf("RESULTADO versao=%s P=%d n=%d delta=- yield=%s "
               "folhas=%d menor_folha=%d maior_folha=%d tempo=%.6f "
               "verificacao=%s\n",
               envio_radenski ? "balanceada_isend" : "balanceada", P, n,
               yield ? yield : "nd", P, menor, maior, tempo,
               erros == 0 ? "OK" : "ERRO");
        free(folhas);
    }

    free(vetor);
    free(aux);
    MPI_Finalize();
    return erros != 0;
}
