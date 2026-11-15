/*
 * T3: divisão e conquista com MPI, versão ORIGINAL (Figura 1 do enunciado).
 *
 * SPMD: todos os processos executam este programa; o papel de cada um sai do
 * rank. A árvore é binária e completa, numerada como um heap:
 *
 *        pai(r) = (r - 1) / 2     filhos(r) = 2r + 1 e 2r + 2
 *        nível(r) = floor(log2(r + 1))
 *
 * Quem recebe um pedaço maior que delta divide: envia as duas metades aos
 * filhos, espera os dois resultados e intercala. Quem recebe até delta
 * conquista com o Bubble Sort. Só as folhas conquistam; os nós internos ficam
 * parados em MPI_Recv enquanto os filhos trabalham.
 *
 * uso: srun -n P ./paralelo n [delta]
 *   n      tamanho do vetor
 *   delta  limite de conquista; omitido ou 0: delta = ceil(2n / (P + 1)),
 *          que faz a árvore ocupar exatamente P = 2^k - 1 processos
 *
 * Além do tempo, imprime uma linha NIVEL por nível da árvore com a média do
 * tempo de cada fase, para mostrar onde os processos ficam parados.
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "ordenacao.h"

#define TAG_DADOS 1
#define TAG_RESULTADO 2

static int pai(int r) { return (r - 1) / 2; }
static int filho_esq(int r) { return 2 * r + 1; }
static int filho_dir(int r) { return 2 * r + 2; }
static int nivel(int r)
{
    int d = 0;
    while ((1 << (d + 1)) - 1 <= r)
        d++;
    return d;
}

static int processos_da_arvore(int rank, int n, int delta, int P)
{
    if (rank >= P)
        return P + 1;
    if (n <= delta)
        return 1;
    int meio = n / 2;
    int esq = processos_da_arvore(filho_esq(rank), meio, delta, P);
    int dir = processos_da_arvore(filho_dir(rank), n - meio, delta, P);
    if (esq > P || dir > P)
        return P + 1;
    return 1 + esq + dir;
}

enum
{
    M_NIVEL,
    M_TAM,
    M_FOLHA,
    M_ESPERA_PAI,
    M_CONQUISTA,
    M_ESPERA_FILHOS,
    M_INTERCALA,
    N_MEDIDAS
};

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int meu_rank, P;
    MPI_Comm_rank(MPI_COMM_WORLD, &meu_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &P);

    /* ---- parâmetros da linha de comando ---- */
    if (argc < 2 || argc > 3)
    {
        if (meu_rank == 0)
            fprintf(stderr, "uso: %s n [delta]\n", argv[0]);
        MPI_Finalize();
        return 1;
    }
    int n = atoi(argv[1]);
    int delta = (argc == 3) ? atoi(argv[2]) : 0;
    if (n < 1)
    {
        if (meu_rank == 0)
            fprintf(stderr, "n deve ser positivo\n");
        MPI_Finalize();
        return 1;
    }
    if (delta <= 0)
        delta = (int)((2LL * n + P) / (P + 1));

    int usados = processos_da_arvore(0, n, delta, P);
    if (usados != P)
    {
        if (meu_rank == 0)
            fprintf(stderr, "a árvore com n=%d e delta=%d usa %s%d processos, "
                            "mas há %d\n",
                    n, delta, usados > P ? "mais de " : "",
                    usados > P ? P : usados, P);
        MPI_Finalize();
        return 1;
    }

    double med[N_MEDIDAS] = {0};
    med[M_NIVEL] = nivel(meu_rank);

    int *vetor = NULL, *aux = NULL;
    int tam_vetor = 0;
    if (meu_rank == 0)
    {
        tam_vetor = n;
        vetor = malloc((size_t)n * sizeof(int));
        if (n > delta)
            aux = malloc((size_t)n * sizeof(int));
        inicializa(vetor, n);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    if (meu_rank != 0)
    {
        MPI_Status status;
        MPI_Probe(pai(meu_rank), TAG_DADOS, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_INT, &tam_vetor);
        vetor = malloc((size_t)tam_vetor * sizeof(int));
        MPI_Recv(vetor, tam_vetor, MPI_INT, pai(meu_rank), TAG_DADOS,
                 MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    double t = MPI_Wtime();
    med[M_ESPERA_PAI] = t - t0;
    med[M_TAM] = tam_vetor;

    if (tam_vetor <= delta)
    {
        bubble_sort(vetor, tam_vetor);
        med[M_FOLHA] = 1;
        med[M_CONQUISTA] = MPI_Wtime() - t;
    }
    else
    {
        int meio = tam_vetor / 2;
        MPI_Send(&vetor[0], meio, MPI_INT, filho_esq(meu_rank),
                 TAG_DADOS, MPI_COMM_WORLD);
        MPI_Send(&vetor[meio], tam_vetor - meio, MPI_INT, filho_dir(meu_rank),
                 TAG_DADOS, MPI_COMM_WORLD);
        MPI_Recv(&vetor[0], meio, MPI_INT, filho_esq(meu_rank),
                 TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&vetor[meio], tam_vetor - meio, MPI_INT, filho_dir(meu_rank),
                 TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        double t_filhos = MPI_Wtime();
        med[M_ESPERA_FILHOS] = t_filhos - t;

        if (!aux)
            aux = malloc((size_t)tam_vetor * sizeof(int));
        intercala(vetor, tam_vetor, aux);
        med[M_INTERCALA] = MPI_Wtime() - t_filhos;
    }

    double tempo = 0.0;
    if (meu_rank != 0)
        MPI_Send(vetor, tam_vetor, MPI_INT, pai(meu_rank),
                 TAG_RESULTADO, MPI_COMM_WORLD);
    else
        tempo = MPI_Wtime() - t0;

    /* ---- fora da região medida: verificação e relatório ---- */
    double *todas = NULL;
    if (meu_rank == 0)
        todas = malloc((size_t)P * N_MEDIDAS * sizeof(double));
    MPI_Gather(med, N_MEDIDAS, MPI_DOUBLE, todas, N_MEDIDAS, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

    long long erros = 0;
    if (meu_rank == 0)
    {
        erros = verifica(vetor, n);
        const char *yield = getenv("OMPI_MCA_mpi_yield_when_idle");
        if (!yield)
            yield = "nd"; /* não definida */

        int folhas = 0, menor = n, maior = 0;
        for (int r = 0; r < P; r++)
        {
            double *m = &todas[r * N_MEDIDAS];
            if (m[M_FOLHA] > 0)
            {
                folhas++;
                if ((int)m[M_TAM] < menor)
                    menor = (int)m[M_TAM];
                if ((int)m[M_TAM] > maior)
                    maior = (int)m[M_TAM];
            }
        }
        printf("RESULTADO versao=original P=%d n=%d delta=%d yield=%s "
               "folhas=%d menor_folha=%d maior_folha=%d tempo=%.6f "
               "verificacao=%s\n",
               P, n, delta, yield, folhas, menor, maior,
               tempo, erros == 0 ? "OK" : "ERRO");

        int niveis = nivel(P - 1) + 1;
        for (int d = 0; d < niveis; d++)
        {
            double soma[N_MEDIDAS] = {0};
            int cont = 0;
            for (int r = 0; r < P; r++)
            {
                double *m = &todas[r * N_MEDIDAS];
                if ((int)m[M_NIVEL] != d)
                    continue;
                for (int k = 0; k < N_MEDIDAS; k++)
                    soma[k] += m[k];
                cont++;
            }
            for (int k = 0; k < N_MEDIDAS; k++)
                soma[k] /= cont;
            printf("NIVEL versao=original P=%d yield=%s nivel=%d procs=%d "
                   "papel=%s tam=%.0f espera_pai=%.6f conquista=%.6f "
                   "espera_filhos=%.6f intercala=%.6f ocupacao=%.4f\n",
                   P, yield, d, cont, soma[M_FOLHA] > 0 ? "folha" : "interno",
                   soma[M_TAM], soma[M_ESPERA_PAI], soma[M_CONQUISTA],
                   soma[M_ESPERA_FILHOS], soma[M_INTERCALA],
                   (soma[M_CONQUISTA] + soma[M_INTERCALA]) / tempo);
        }
        fflush(stdout);
        free(todas);
    }

    free(vetor);
    free(aux);
    MPI_Finalize();
    return erros != 0;
}
