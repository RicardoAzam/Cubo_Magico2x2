#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "busca.h"
#include "cubo.h"
#include "B_FILA.h"

#define TAM_ESTADO 24
#define NUM_MOVIMENTOS 12
#define MAX_PROFUNDIDADE_DFS 11
#define MAX_PROFUNDIDADE_CAMADAS 8
#define MAX_NOS_BFS 250000

typedef struct
{
    unsigned char valores[TAM_ESTADO];
} Estado;

typedef struct
{
    Estado estado;
    int pai;
    int movimento;
    int profundidade;
} No;

/*
 * Cada par representa uma camada fisica e seus dois sentidos.
 * A escolha do lado feita pelo usuario apenas define qual par sera
 * testado primeiro na busca em profundidade.
 */
static const Movimento movimentos[NUM_MOVIMENTOS] =
{
    {R, 'C', 1}, {R, 'C', 0},
    {R, 'B', 1}, {R, 'B', 0},
    {F, 'D', 1}, {F, 'D', 0},
    {F, 'E', 1}, {F, 'E', 0},
    {U, 'B', 1}, {U, 'B', 0},
    {U, 'C', 1}, {U, 'C', 0}
};

static unsigned long long nosExploradosDFS;
static int caminhoDFS[MAX_PROFUNDIDADE_DFS];
static Estado estadoSolucaoDFS;

static unsigned long long nosExploradosCamadas;
static int caminhoCamada[MAX_PROFUNDIDADE_CAMADAS];
static Estado estadoCamadaEncontrada;

static double tempoAtual(void)
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

static char letraFace(int face)
{
    switch (face)
    {
        case U: return 'U';
        case D: return 'D';
        case F: return 'F';
        case B: return 'B';
        case L: return 'L';
        case R: return 'R';
        default: return '?';
    }
}

static const char *nomeFaixa(char faixa)
{
    switch (faixa)
    {
        case 'C': return "Cima";
        case 'B': return "Baixo";
        case 'E': return "Esquerda";
        case 'D': return "Direita";
        default: return "?";
    }
}

static void copiarCubeParaEstado(Estado *estado)
{
    for (int f = 0; f < NUM_FACES; f++)
        for (int p = 0; p < STICKERS_POR_FACE; p++)
            estado->valores[f * STICKERS_POR_FACE + p] =
                (unsigned char)cube[f][p];
}

static void copiarEstadoParaCube(const Estado *estado)
{
    for (int f = 0; f < NUM_FACES; f++)
        for (int p = 0; p < STICKERS_POR_FACE; p++)
            cube[f][p] = estado->valores[f * STICKERS_POR_FACE + p];
}

static int estadosIguais(const Estado *a, const Estado *b)
{
    return memcmp(a->valores, b->valores, TAM_ESTADO) == 0;
}

/* Usa a movimentacao oficial do modulo do cubo para transformar um estado. */
static void aplicarMovimento(Estado *estado, int indiceMovimento)
{
    copiarEstadoParaCube(estado);

    moverCamada(
        movimentos[indiceMovimento].face,
        movimentos[indiceMovimento].faixa,
        movimentos[indiceMovimento].sentido
    );

    copiarCubeParaEstado(estado);
}

static int estadoResolvidoBusca(const Estado *estado)
{
    for (int f = 0; f < NUM_FACES; f++)
    {
        int cor = estado->valores[f * STICKERS_POR_FACE];

        for (int p = 1; p < STICKERS_POR_FACE; p++)
            if (estado->valores[f * STICKERS_POR_FACE + p] != cor)
                return 0;
    }

    return 1;
}

/*
 * Objetivo da primeira etapa da busca por camadas.
 * Para manter a ideia simples, consideramos a camada superior pronta
 * quando os quatro stickers da face U possuem a mesma cor.
 */
static int camadaSuperiorResolvida(const Estado *estado)
{
    int cor = estado->valores[U * STICKERS_POR_FACE];

    for (int p = 1; p < STICKERS_POR_FACE; p++)
        if (estado->valores[U * STICKERS_POR_FACE + p] != cor)
            return 0;

    return 1;
}

static int objetivoAtingido(const Estado *estado, int objetivo)
{
    if (objetivo == 1)
        return camadaSuperiorResolvida(estado);

    return estadoResolvidoBusca(estado);
}

static void montarOrdemMovimentos(int ladoInicial, int ordem[NUM_MOVIMENTOS])
{
    int primeiroPar;

    switch (ladoInicial)
    {
        case U: primeiroPar = 10; break;
        case D: primeiroPar = 2; break;
        case F: primeiroPar = 8; break;
        case B: primeiroPar = 0; break;
        case L: primeiroPar = 6; break;
        case R: primeiroPar = 4; break;
        default: primeiroPar = 0; break;
    }

    int posicao = 0;

    for (int deslocamento = 0; deslocamento < NUM_MOVIMENTOS; deslocamento += 2)
    {
        int indice = (primeiroPar + deslocamento) % NUM_MOVIMENTOS;
        ordem[posicao++] = indice;
        ordem[posicao++] = indice + 1;
    }
}

static void imprimirSolucao(const int caminho[], int quantidade)
{
    if (quantidade == 0)
    {
        printf("Cubo ja estava resolvido.\n");
        return;
    }

    for (int i = 0; i < quantidade; i++)
    {
        Movimento m = movimentos[caminho[i]];

        printf("%d. %c + %s + %s",
               i + 1,
               letraFace(m.face),
               nomeFaixa(m.faixa),
               m.sentido ? "Horario" : "Anti-horario");

        if (i < quantidade - 1)
            printf(" ->\n");
        else
            printf("\n");
    }
}

/* DFS simples com aprofundamento progressivo. */
static int dfs(
    Estado estado,
    int profundidade,
    int limite,
    int ultimoMovimento,
    const int ordem[NUM_MOVIMENTOS],
    int objetivo)
{
    nosExploradosDFS++;

    if (objetivoAtingido(&estado, objetivo))
    {
        estadoSolucaoDFS = estado;
        return 1;
    }

    if (profundidade >= limite)
        return 0;

    for (int i = 0; i < NUM_MOVIMENTOS; i++)
    {
        int movimento = ordem[i];

        /* Evita fazer imediatamente o movimento inverso. */
        if (ultimoMovimento != -1 && (movimento ^ 1) == ultimoMovimento)
            continue;

        Estado proximo = estado;
        aplicarMovimento(&proximo, movimento);
        caminhoDFS[profundidade] = movimento;

        if (dfs(proximo,
                profundidade + 1,
                limite,
                movimento,
                ordem,
                objetivo))
        {
            return 1;
        }
    }

    return 0;
}

int resolverEmProfundidade(int ladoInicial)
{
    if (ladoInicial < 0 || ladoInicial >= NUM_FACES)
        return -1;

    Estado inicial;
    copiarCubeParaEstado(&inicial);

    int ordem[NUM_MOVIMENTOS];
    montarOrdemMovimentos(ladoInicial, ordem);

    double inicio = tempoAtual();
    nosExploradosDFS = 0;

    for (int limite = 0; limite <= MAX_PROFUNDIDADE_DFS; limite++)
    {
        if (dfs(inicial, 0, limite, -1, ordem, 0))
        {
            double fim = tempoAtual();

            copiarEstadoParaCube(&estadoSolucaoDFS);

            printf("\nSolucao encontrada por busca em profundidade!\n");
            printf("Lado priorizado: %c\n", letraFace(ladoInicial));
            printf("Profundidade: %d movimento(s)\n", limite);
            printf("Nos explorados: %llu\n",
                   (unsigned long long)nosExploradosDFS);
            printf("Tempo da busca: %.6f segundos\n", fim - inicio);
            printf("Ordem de solucao:\n");
            imprimirSolucao(caminhoDFS, limite);

            return limite;
        }
    }

    copiarEstadoParaCube(&inicial);

    printf("\nNao foi encontrada uma solucao ate a profundidade %d.\n",
           MAX_PROFUNDIDADE_DFS);
    printf("Nos explorados: %llu\n",
           (unsigned long long)nosExploradosDFS);
    printf("Tempo da busca: %.6f segundos\n",
           tempoAtual() - inicio);

    return 0;
}

/* ============================================================
 * BUSCA POR CAMADAS
 * ============================================================ */

static int dfsCamadas(
    Estado estado,
    int profundidade,
    int limite,
    int ultimoMovimento,
    const int ordem[NUM_MOVIMENTOS])
{
    nosExploradosCamadas++;

    if (camadaSuperiorResolvida(&estado))
    {
        estadoCamadaEncontrada = estado;
        return 1;
    }

    if (profundidade >= limite)
        return 0;

    for (int i = 0; i < NUM_MOVIMENTOS; i++)
    {
        int movimento = ordem[i];

        if (ultimoMovimento != -1 && (movimento ^ 1) == ultimoMovimento)
            continue;

        Estado proximo = estado;
        aplicarMovimento(&proximo, movimento);
        caminhoCamada[profundidade] = movimento;

        if (dfsCamadas(proximo,
                       profundidade + 1,
                       limite,
                       movimento,
                       ordem))
        {
            return 1;
        }
    }

    return 0;
}

int resolverPorCamadas(void)
{
    Estado inicial;
    copiarCubeParaEstado(&inicial);

    int ordem[NUM_MOVIMENTOS];
    montarOrdemMovimentos(U, ordem);

    double inicioTotal = tempoAtual();
    double inicioPrimeira = tempoAtual();
    nosExploradosCamadas = 0;

    int profundidadePrimeira = -1;

    for (int limite = 0; limite <= MAX_PROFUNDIDADE_CAMADAS; limite++)
    {
        if (dfsCamadas(inicial, 0, limite, -1, ordem))
        {
            profundidadePrimeira = limite;
            break;
        }
    }

    double fimPrimeira = tempoAtual();

    if (profundidadePrimeira == -1)
    {
        copiarEstadoParaCube(&inicial);

        printf("\nNao foi encontrada a camada superior ate a profundidade %d.\n",
               MAX_PROFUNDIDADE_CAMADAS);
        printf("Nos explorados na primeira etapa: %llu\n",
               (unsigned long long)nosExploradosCamadas);
        printf("Tempo da primeira etapa: %.6f segundos\n",
               fimPrimeira - inicioPrimeira);
        printf("Tempo total: %.6f segundos\n",
               fimPrimeira - inicioTotal);

        return 0;
    }

    Estado estadoIntermediario = estadoCamadaEncontrada;

    /* Guarda a primeira parte porque a segunda busca reutiliza o caminhoDFS. */
    int primeiraParte[MAX_PROFUNDIDADE_CAMADAS];
    for (int i = 0; i < profundidadePrimeira; i++)
        primeiraParte[i] = caminhoCamada[i];

    /* A segunda etapa usa a mesma DFS simples, agora com objetivo final. */
    nosExploradosDFS = 0;
    double inicioSegunda = tempoAtual();
    int profundidadeSegunda = -1;

    for (int limite = 0; limite <= MAX_PROFUNDIDADE_DFS; limite++)
    {
        if (dfs(estadoIntermediario, 0, limite, -1, ordem, 0))
        {
            profundidadeSegunda = limite;
            break;
        }
    }

    double fimSegunda = tempoAtual();

    if (profundidadeSegunda == -1)
    {
        copiarEstadoParaCube(&inicial);

        printf("\nNao foi encontrada a solucao na segunda etapa.\n");
        printf("Primeira etapa (camada superior): %d movimento(s)\n",
               profundidadePrimeira);
        printf("Nos explorados na primeira etapa: %llu\n",
               (unsigned long long)nosExploradosCamadas);
        printf("Tempo da primeira etapa: %.6f segundos\n",
               fimPrimeira - inicioPrimeira);
        printf("Nos explorados na segunda etapa: %llu\n",
               (unsigned long long)nosExploradosDFS);
        printf("Tempo da segunda etapa: %.6f segundos\n",
               fimSegunda - inicioSegunda);
        printf("Tempo total: %.6f segundos\n",
               fimSegunda - inicioTotal);

        return 0;
    }

    copiarEstadoParaCube(&estadoSolucaoDFS);

    printf("\nSolucao encontrada por busca por camadas!\n");
    printf("Primeira etapa (camada superior): %d movimento(s)\n",
           profundidadePrimeira);
    printf("Segunda etapa: %d movimento(s)\n", profundidadeSegunda);
    printf("Total: %d movimento(s)\n",
           profundidadePrimeira + profundidadeSegunda);
    printf("Nos explorados na primeira etapa: %llu\n",
           (unsigned long long)nosExploradosCamadas);
    printf("Nos explorados na segunda etapa: %llu\n",
           (unsigned long long)nosExploradosDFS);
    printf("Tempo da primeira etapa: %.6f segundos\n",
           fimPrimeira - inicioPrimeira);
    printf("Tempo da segunda etapa: %.6f segundos\n",
           fimSegunda - inicioSegunda);
    printf("Tempo total: %.6f segundos\n",
           fimSegunda - inicioTotal);
    printf("Ordem de solucao:\n");

    for (int i = 0; i < profundidadePrimeira; i++)
    {
        Movimento m = movimentos[primeiraParte[i]];

        printf("%d. %c + %s + %s ->\n",
               i + 1,
               letraFace(m.face),
               nomeFaixa(m.faixa),
               m.sentido ? "Horario" : "Anti-horario");
    }

    if (profundidadeSegunda == 0)
    {
        if (profundidadePrimeira == 0)
            printf("Cubo ja estava resolvido.\n");
    }
    else
    {
        for (int i = 0; i < profundidadeSegunda; i++)
        {
            Movimento m = movimentos[caminhoDFS[i]];
            int numero = profundidadePrimeira + i + 1;

            printf("%d. %c + %s + %s",
                   numero,
                   letraFace(m.face),
                   nomeFaixa(m.faixa),
                   m.sentido ? "Horario" : "Anti-horario");

            if (i < profundidadeSegunda - 1)
                printf(" ->\n");
            else
                printf("\n");
        }
    }

    return profundidadePrimeira + profundidadeSegunda;
}

/* ============================================================
 * BUSCA EM LARGURA
 * ============================================================ */

static int estadoJaVisitado(const Estado *estado,
                            const No nos[],
                            int quantidade)
{
    for (int i = 0; i < quantidade; i++)
    {
        if (estadosIguais(estado, &nos[i].estado))
            return 1;
    }

    return 0;
}

int resolverEmLargura(void)
{
    Estado inicial;
    copiarCubeParaEstado(&inicial);

    No *nos = (No *)malloc(sizeof(No) * MAX_NOS_BFS);
    if (nos == NULL)
    {
        printf("\nNao foi possivel reservar memoria para a busca em largura.\n");
        return -1;
    }

    Fila *fila = CriaFila();
    if (fila == NULL)
    {
        free(nos);
        printf("\nNao foi possivel criar a fila da busca em largura.\n");
        return -1;
    }

    double inicio = tempoAtual();
    unsigned long long nosExpandidos = 0;
    int quantidade = 1;
    int objetivo = -1;

    nos[0].estado = inicial;
    nos[0].pai = -1;
    nos[0].movimento = -1;
    nos[0].profundidade = 0;

    InsereFila(fila, 0);

    while (fila->ini != NULL)
    {
        int indiceAtual = removeInicio(fila);
        if (indiceAtual < 0)
            break;

        No atual = nos[indiceAtual];
        nosExpandidos++;

        if (estadoResolvidoBusca(&atual.estado))
        {
            objetivo = indiceAtual;
            break;
        }

        if (atual.profundidade >= MAX_PROFUNDIDADE_DFS)
            continue;

        for (int movimento = 0; movimento < NUM_MOVIMENTOS; movimento++)
        {
            if (atual.movimento != -1 &&
                (movimento ^ 1) == atual.movimento)
            {
                continue;
            }

            Estado proximo = atual.estado;
            aplicarMovimento(&proximo, movimento);

            if (estadoJaVisitado(&proximo, nos, quantidade))
                continue;

            if (quantidade >= MAX_NOS_BFS)
            {
                printf("\nLimite de %d estados atingido pela busca em largura.\n",
                       MAX_NOS_BFS);
                printf("A busca foi encerrada.\n");
                destroiFila(fila);
                free(nos);
                return 0;
            }

            nos[quantidade].estado = proximo;
            nos[quantidade].pai = indiceAtual;
            nos[quantidade].movimento = movimento;
            nos[quantidade].profundidade = atual.profundidade + 1;

            InsereFila(fila, quantidade);
            quantidade++;
        }
    }

    double fim = tempoAtual();

    if (objetivo == -1)
    {
        copiarEstadoParaCube(&inicial);

        printf("\nNao foi encontrada uma solucao pela busca em largura.\n");
        printf("Estados visitados: %d\n", quantidade);
        printf("Nos expandidos: %llu\n",
               (unsigned long long)nosExpandidos);
        printf("Tempo da busca: %.6f segundos\n", fim - inicio);

        destroiFila(fila);
        free(nos);
        return 0;
    }

    int profundidade = nos[objetivo].profundidade;
    int *caminho = NULL;

    if (profundidade > 0)
        caminho = (int *)malloc(sizeof(int) * profundidade);

    int atual = objetivo;
    for (int i = profundidade - 1; i >= 0; i--)
    {
        caminho[i] = nos[atual].movimento;
        atual = nos[atual].pai;
    }

    copiarEstadoParaCube(&nos[objetivo].estado);

    printf("\nSolucao encontrada por busca em largura!\n");
    printf("Profundidade: %d movimento(s)\n", profundidade);
    printf("Estados visitados: %d\n", quantidade);
    printf("Nos expandidos: %llu\n",
           (unsigned long long)nosExpandidos);
    printf("Tempo da busca: %.6f segundos\n", fim - inicio);
    printf("Ordem de solucao:\n");
    imprimirSolucao(caminho, profundidade);

    free(caminho);
    destroiFila(fila);
    free(nos);

    return profundidade;
}
