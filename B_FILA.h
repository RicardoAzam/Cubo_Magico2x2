#ifndef B_FILA_H
#define B_FILA_H

/* Biblioteca de fila do projeto do Cubo Magico 2x2.
 * A fila sera reutilizada pela busca em largura. */

typedef struct nos
{
    int info;
    struct nos *prox;
} Nos;

typedef struct fila
{
    Nos *ini;
    Nos *fim;
} Fila;

Fila *CriaFila(void);
Nos *ins_fim(Nos *fim, int A);
void InsereFila(Fila *f, int v);
int removeInicio(Fila *f);
void rotacionaFila(Fila *f);
void destroiFila(Fila *f);

#endif
