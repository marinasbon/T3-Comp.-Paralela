/*
 * T3: ordenação de um vetor, versão sequencial (referências T1 e T1^delta).
 *
 * Adaptado do sequencial.c distribuído com o enunciado. O vetor começa em
 * ordem decrescente (pior caso do Bubble Sort). Três modos:
 *
 *   ./sequencial n           ordena o vetor inteiro com o Bubble Sort (T1)
 *   ./sequencial n delta     percorre a árvore da versão ORIGINAL em um só
 *                            processo: divide enquanto o pedaço passa de
 *                            delta, ordena as folhas com o Bubble Sort e
 *                            intercala na volta (T1^delta)
 *   ./sequencial n -b P      percorre a árvore da versão BALANCEADA com P
 *                            processos em um só processo (T1^delta da
 *                            balanceada; com P que não é potência de 2 as
 *                            folhas têm tamanhos diferentes e nenhum delta
 *                            único reproduz a árvore)
 *
 * Além do tempo, imprime o número de folhas e o menor e o maior tamanho de
 * folha, para mostrar que a referência faz o mesmo trabalho da versão
 * paralela correspondente (que imprime os mesmos campos).
 *
 * Compilação: mesmas flags das versões paralelas (ver Makefile).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ordenacao.h"

/* Estatística das folhas, para comparar com a versão paralela. */
static int num_folhas = 0, menor_folha = -1, maior_folha = 0;

static void registra_folha(int n)
{
    num_folhas++;
    if (menor_folha < 0 || n < menor_folha) menor_folha = n;
    if (n > maior_folha) maior_folha = n;
}

/*
 * A árvore da versão original, percorrida em um só processo. A metade
 * esquerda tem n/2 elementos, como a que o pai envia ao filho da esquerda.
 */
static void ordena(int *v, int n, int delta, int *aux)
{
    if (n <= delta) {                       /* conquista */
        registra_folha(n);
        bubble_sort(v, n);
        return;
    }
    int meio = n / 2;                       /* divisão */
    ordena(v, meio, delta, aux);
    ordena(v + meio, n - meio, delta, aux + meio);
    intercala(v, n, aux);                   /* combinação */
}

/*
 * A árvore da versão balanceada (balanceado.c), percorrida em um só processo.
 * O processo 'rank' no nível 'nivel' tem como ajudante rank + 2^nivel; se o
 * ajudante não existe (>= P), conquista. Senão fica com a metade esquerda
 * (continua no nível seguinte) e o ajudante recebe a direita, começando no
 * mesmo nível seguinte. A recursão é a mesma de ordena_paralelo() em
 * balanceado.c, trocando a mensagem por uma chamada.
 */
static void ordena_bal(int *v, int n, int rank, int nivel, int P, int *aux)
{
    int ajudante = rank + (1 << nivel);
    if (ajudante >= P) {                    /* conquista */
        registra_folha(n);
        bubble_sort(v, n);
        return;
    }
    int meio = n / 2;
    ordena_bal(v, meio, rank, nivel + 1, P, aux);                 /* fica */
    ordena_bal(v + meio, n - meio, ajudante, nivel + 1, P, aux + meio); /* envia */
    intercala(v, n, aux);
}

static double agora(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

int main(int argc, char **argv)
{
    int balanceada = (argc == 4 && strcmp(argv[2], "-b") == 0);
    if (argc != 2 && argc != 3 && !balanceada) {
        fprintf(stderr, "uso: %s n [delta | -b P]\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    int delta = n, P = 1;
    if (balanceada)
        P = atoi(argv[3]);
    else if (argc == 3)
        delta = atoi(argv[2]);              /* sem delta: vetor inteiro */
    if (n < 1 || delta < 1 || P < 1) {
        fprintf(stderr, "n, delta e P devem ser positivos\n");
        return 1;
    }

    /* Na pilha, 10^6 ints estouram: aloca no heap. */
    int *v = malloc((size_t)n * sizeof(int));
    int *aux = malloc((size_t)n * sizeof(int));
    if (!v || !aux) {
        fprintf(stderr, "sem memória para n = %d\n", n);
        return 1;
    }

    inicializa(v, n);                       /* fora da região medida */

    double t0 = agora();
    if (balanceada)
        ordena_bal(v, n, 0, 0, P, aux);     /* região medida */
    else
        ordena(v, n, delta, aux);           /* com delta = n, é só o Bubble Sort */
    double t1 = agora();

    long long erros = verifica(v, n);

    /* Uma linha por execução, no mesmo formato chave=valor das paralelas. */
    if (balanceada)
        printf("RESULTADO versao=seq_bal P=%d n=%d delta=- ", P, n);
    else
        printf("RESULTADO versao=seq P=1 n=%d delta=%d ", n, delta);
    printf("folhas=%d menor_folha=%d maior_folha=%d tempo=%.6f verificacao=%s\n",
           num_folhas, menor_folha, maior_folha, t1 - t0,
           erros == 0 ? "OK" : "ERRO");

    free(v);
    free(aux);
    return erros != 0;
}
