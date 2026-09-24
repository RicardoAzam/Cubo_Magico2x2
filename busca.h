#ifndef BUSCA_H
#define BUSCA_H

#include "cubo.h"

/* Busca em profundidade.
 * Retorno: numero de movimentos encontrados, -1 para estado invalido
 * ou 0 quando nao encontra dentro do limite configurado. */
int resolverEmProfundidade(int ladoInicial);

/* Busca em largura.
 * Retorno: numero de movimentos encontrados, -1 para estado invalido
 * ou 0 quando nao encontra dentro dos limites de memoria configurados. */
int resolverEmLargura(void);

/* Busca por camadas. Resolve primeiro a camada superior e depois o cubo inteiro. */
int resolverPorCamadas(void);

#endif
