#include <stdlib.h>
#include "B_FILA.h"

Fila *CriaFila(void)
{
    Fila *f = (Fila *)malloc(sizeof(Fila));
    if (f == NULL) return NULL;

    f->ini = NULL;
    f->fim = NULL;
    return f;
}

Nos *ins_fim(Nos *fim, int A)
{
    Nos *p = (Nos *)malloc(sizeof(Nos));
    if (p == NULL) return NULL;

    p->info = A;
    p->prox = NULL;

    if (fim != NULL)
        fim->prox = p;

    return p;
}

void InsereFila(Fila *f, int v)
{
    if (f == NULL) return;

    Nos *novo = ins_fim(f->fim, v);
    if (novo == NULL) return;

    f->fim = novo;
    if (f->ini == NULL)
        f->ini = novo;
}

int removeInicio(Fila *f)
{
    if (f == NULL || f->ini == NULL) return -1;

    Nos *removido = f->ini;
    int valor = removido->info;

    f->ini = removido->prox;
    if (f->ini == NULL)
        f->fim = NULL;

    free(removido);
    return valor;
}

void rotacionaFila(Fila *f)
{
    if (f == NULL || f->ini == NULL) return;

    int valor = removeInicio(f);
    if (valor != -1)
        InsereFila(f, valor);
}

void destroiFila(Fila *f)
{
    if (f == NULL) return;

    Nos *atual = f->ini;
    while (atual != NULL)
    {
        Nos *prox = atual->prox;
        free(atual);
        atual = prox;
    }

    free(f);
}
