/*
 * T3: rotinas comuns às três versões (sequencial, original e balanceada).
 *
 * bubble_sort() e intercala() são as do sequencial.c distribuído com o
 * enunciado, com o mesmo corpo; só passaram a ser static, por estarem num
 * cabeçalho. Ficam aqui para que as versões sequencial e paralelas usem
 * exatamente o mesmo código de conquista e de combinação (e, portanto, façam
 * o mesmo trabalho).
 */
#ifndef ORDENACAO_H
#define ORDENACAO_H

#include <string.h>

static void bubble_sort(int *v, int n)
{
    int trocou = 1;
    for (int fim = n - 1; fim > 0 && trocou; fim--)
    {
        trocou = 0;
        for (int i = 0; i < fim; i++)
        {
            if (v[i] > v[i + 1])
            {
                int t = v[i];
                v[i] = v[i + 1];
                v[i + 1] = t;
                trocou = 1;
            }
        }
    }
}

static void intercala(int *v, int n, int *aux)
{
    int meio = n / 2;
    int i = 0, j = meio, k = 0;

    while (i < meio && j < n)
        aux[k++] = (v[i] <= v[j]) ? v[i++] : v[j++];
    while (i < meio)
        aux[k++] = v[i++];
    while (j < n)
        aux[k++] = v[j++];

    memcpy(v, aux, (size_t)n * sizeof(int));
}

static void inicializa(int *v, int n)
{
    for (int i = 0; i < n; i++)
        v[i] = n - i;
}

static long long verifica(const int *v, int n)
{
    long long erros = 0;
    for (int i = 0; i < n; i++)
        if (v[i] != i + 1)
            erros++;
    return erros;
}

#endif
