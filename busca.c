#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "busca.h"
#include "B_FILA.h"

/* ============================================================
 * BUSCA DE SOLUCAO EM PROFUNDIDADE (DFS + aprofundamento)
 * ============================================================
 *
 * A DFS explora uma tentativa ate a profundidade atual antes de
 * voltar (backtracking) e testar a proxima possibilidade.
 *
 * Para o cubo 2x2 existem 12 movimentos fisicos diferentes:
 * cada uma das 6 camadas pode ser girada em 2 sentidos.
 * A entrada do usuario continua usando lado + faixa + sentido;
 * aqui usamos apenas uma representacao canonica de cada camada.
 */

typedef struct
{
    int face;
    char faixa;
    int sentido;
} MovimentoSolucao;

#define NUM_MOVIMENTOS_SOLUCAO 12
#define MAX_PROFUNDIDADE_SOLUCAO 11
#define TAM_HASH_SOLUCAO (1u << 20)
#define MAX_METAS_SOLUCAO 48
#define TAM_PDB_CANTO 331776

typedef struct
{
    uint64_t hash;
    unsigned char profundidade;
    signed char ultimo;
    unsigned char usado;
} EntradaHashSolucao;

static const MovimentoSolucao movimentosSolucao[NUM_MOVIMENTOS_SOLUCAO] =
{
    /* camada y = +1 (cima) */
    {R, 'C', 1}, {R, 'C', 0},
    /* camada y = -1 (baixo) */
    {R, 'B', 1}, {R, 'B', 0},
    /* camada x = +1 (direita) */
    {F, 'D', 1}, {F, 'D', 0},
    /* camada x = -1 (esquerda) */
    {F, 'E', 1}, {F, 'E', 0},
    /* camada z = +1 (frente) */
    {U, 'B', 1}, {U, 'B', 0},
    /* camada z = -1 (tras) */
    {U, 'C', 1}, {U, 'C', 0}
};

static unsigned char permStickerSolucao[NUM_MOVIMENTOS_SOLUCAO][24];
static unsigned char permCantoSolucao[NUM_MOVIMENTOS_SOLUCAO][8];
static unsigned char distPermCanto[40320];
static EntradaHashSolucao *tabelaHashSolucao = NULL;
static int solverPreparado = 0;
static int cantosSticker[8][3];
static int numMetasSolucao = 0;
static int idCantoPorMascaraSolucao[64];
static unsigned char metasSolucao[MAX_METAS_SOLUCAO][8];
static int coresMetasSolucao[MAX_METAS_SOLUCAO][6];
static unsigned char pdbCantoA[TAM_PDB_CANTO];
static unsigned char pdbCantoB[TAM_PDB_CANTO];
static int pdbPreparadoSolucao = 0;
static unsigned long long nosExploradosSolucao = 0;
static int caminhoSolucao[MAX_PROFUNDIDADE_SOLUCAO];
static int ladoPrioritarioSolucao = U;

/* Retorna o tempo de execucao do processo em segundos. */
static double tempoAtualSolucao(void)
{
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

static char letraFaceSolucao(int face)
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

static const char *nomeFaixaSolucao(char faixa)
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


static uint64_t hashEstadoSolucao(void)
{
    uint64_t h = UINT64_C(1469598103934665603);

    for (int f = 0; f < NUM_FACES; f++)
    {
        for (int p = 0; p < STICKERS_POR_FACE; p++)
        {
            h ^= (uint64_t)(cube[f][p] & 7);
            h *= UINT64_C(1099511628211);
        }
    }

    return h;
}

/* Gera a permutacao exata de stickers para cada um dos 12 movimentos. */
static void prepararPermutacoesStickerSolucao(void)
{
    for (int m = 0; m < NUM_MOVIMENTOS_SOLUCAO; m++)
    {
        int face = movimentosSolucao[m].face;
        char faixa = movimentosSolucao[m].faixa;
        int sentidoHorario = movimentosSolucao[m].sentido;

        Vetor3 eixo = eixoDaFaixa(face, faixa);
        int sinal = sinalHorarioDaFaixa(face, faixa);

        if (!sentidoHorario)
            sinal = -sinal;

        for (int f = 0; f < NUM_FACES; f++)
        {
            for (int p = 0; p < STICKERS_POR_FACE; p++)
            {
                int origemIndex = indiceStickerLinear(f, p);
                PosicaoSticker origem = obterPosicaoSticker(f, p);

                int pertence =
                    origem.posicao.x * eixo.x +
                    origem.posicao.y * eixo.y +
                    origem.posicao.z * eixo.z == 1;

                if (!pertence)
                {
                    permStickerSolucao[m][origemIndex] = (unsigned char)origemIndex;
                    continue;
                }

                PosicaoSticker destino;
                destino.posicao = rotacionar90(origem.posicao, eixo, sinal);
                destino.normal = rotacionar90(origem.normal, eixo, sinal);

                int novaFace, novaPos;
                encontrarSticker(destino.posicao, destino.normal,
                                 &novaFace, &novaPos);

                if (novaFace == -1 || novaPos == -1)
                {
                    permStickerSolucao[m][origemIndex] = (unsigned char)origemIndex;
                }
                else
                {
                    permStickerSolucao[m][origemIndex] =
                        (unsigned char)indiceStickerLinear(novaFace, novaPos);
                }
            }
        }
    }
}

/* Gera a permutacao dos 8 cubinhos (cantos) de cada movimento. */
static void prepararPermutacoesCantoSolucao(void)
{
    for (int m = 0; m < NUM_MOVIMENTOS_SOLUCAO; m++)
    {
        int face = movimentosSolucao[m].face;
        char faixa = movimentosSolucao[m].faixa;
        int sentidoHorario = movimentosSolucao[m].sentido;

        Vetor3 eixo = eixoDaFaixa(face, faixa);
        int sinal = sinalHorarioDaFaixa(face, faixa);

        if (!sentidoHorario)
            sinal = -sinal;

        for (int c = 0; c < 8; c++)
        {
            Vetor3 origem = coordenadasCantos[c];
            int pertence =
                origem.x * eixo.x +
                origem.y * eixo.y +
                origem.z * eixo.z == 1;

            if (!pertence)
            {
                permCantoSolucao[m][c] = (unsigned char)c;
                continue;
            }

            Vetor3 destino = rotacionar90(origem, eixo, sinal);
            int novoCanto = indiceCantoPorCoordenada(destino);

            permCantoSolucao[m][c] =
                (unsigned char)((novoCanto == -1) ? c : novoCanto);
        }
    }
}

static int rankPermutacao8(const unsigned char p[8])
{
    int rank = 0;

    for (int i = 0; i < 8; i++)
    {
        int menores = 0;

        for (int j = i + 1; j < 8; j++)
            if (p[j] < p[i])
                menores++;

        rank += menores * (i == 0 ? 5040 :
                           i == 1 ? 720 :
                           i == 2 ? 120 :
                           i == 3 ? 24 :
                           i == 4 ? 6 :
                           i == 5 ? 2 : 1);
    }

    return rank;
}

static void unrankPermutacao8(int rank, unsigned char p[8])
{
    int disponiveis[8];
    int n = 8;

    for (int i = 0; i < 8; i++)
        disponiveis[i] = i;

    static const int fatoriais[8] =
        {5040, 720, 120, 24, 6, 2, 1, 1};

    for (int i = 0; i < 8; i++)
    {
        int bloco = fatoriais[i];
        int indice = (bloco == 0) ? 0 : rank / bloco;
        rank %= bloco;

        p[i] = (unsigned char)disponiveis[indice];

        for (int j = indice; j < n - 1; j++)
            disponiveis[j] = disponiveis[j + 1];

        n--;
    }
}

static void aplicarPermutacaoCantoAoEstado(
    const unsigned char entrada[8],
    int movimento,
    unsigned char saida[8])
{
    for (int c = 0; c < 8; c++)
        saida[permCantoSolucao[movimento][c]] = entrada[c];
}

/* Distancia minima considerando apenas a permutacao dos cantos. */
static void prepararDistanciasCantoSolucao(void)
{
    for (int i = 0; i < 40320; i++)
        distPermCanto[i] = 255;

    unsigned short fila[40320];
    int inicio = 0;
    int fim = 0;

    unsigned char identidade[8] = {0,1,2,3,4,5,6,7};
    int rankInicial = rankPermutacao8(identidade);

    fila[fim++] = (unsigned short)rankInicial;
    distPermCanto[rankInicial] = 0;

    while (inicio < fim)
    {
        int rankAtual = fila[inicio++];
        unsigned char atual[8];
        unrankPermutacao8(rankAtual, atual);

        for (int m = 0; m < NUM_MOVIMENTOS_SOLUCAO; m++)
        {
            unsigned char proximo[8];
            aplicarPermutacaoCantoAoEstado(atual, m, proximo);

            int rankProximo = rankPermutacao8(proximo);

            if (distPermCanto[rankProximo] == 255)
            {
                distPermCanto[rankProximo] =
                    (unsigned char)(distPermCanto[rankAtual] + 1);
                fila[fim++] = (unsigned short)rankProximo;
            }
        }
    }
}

/* Monta os 8 conjuntos de 3 cores presentes nos cubinhos atuais. */
static int prepararCantosStickerSolucao(void)
{
    for (int c = 0; c < 8; c++)
        for (int k = 0; k < 3; k++)
            cantosSticker[c][k] = -1;

    for (int f = 0; f < NUM_FACES; f++)
    {
        for (int p = 0; p < STICKERS_POR_FACE; p++)
        {
            PosicaoSticker s = obterPosicaoSticker(f, p);
            int c = indiceCantoPorCoordenada(s.posicao);

            if (c == -1)
                continue;

            for (int k = 0; k < 3; k++)
            {
                if (cantosSticker[c][k] == -1)
                {
                    cantosSticker[c][k] = indiceStickerLinear(f, p);
                    break;
                }
            }
        }
    }

    for (int c = 0; c < 8; c++)
        for (int k = 0; k < 3; k++)
            if (cantosSticker[c][k] == -1)
                return 0;

    return 1;
}

static int mascaraCantoAtual(int c)
{
    int mascara = 0;

    for (int k = 0; k < 3; k++)
    {
        int indice = cantosSticker[c][k];
        int face = indice / STICKERS_POR_FACE;
        int pos = indice % STICKERS_POR_FACE;
        (void)pos;

        int cor = cube[face][pos];
        if (cor < 1 || cor > 6)
            return -1;

        mascara |= 1 << (cor - 1);
    }

    return mascara;
}

static int mascaraCantoMeta(const int coresFace[6], int c)
{
    int mascara = 0;
    const Vetor3 v = coordenadasCantos[c];

    if (v.y == 1) mascara |= 1 << (coresFace[U] - 1);
    else           mascara |= 1 << (coresFace[D] - 1);

    if (v.z == 1) mascara |= 1 << (coresFace[F] - 1);
    else           mascara |= 1 << (coresFace[B] - 1);

    if (v.x == 1) mascara |= 1 << (coresFace[R] - 1);
    else           mascara |= 1 << (coresFace[L] - 1);

    return mascara;
}

static int idCantoPorMascara(const int mascaras[8], int mascara)
{
    for (int i = 0; i < 8; i++)
        if (mascaras[i] == mascara)
            return i;

    return -1;
}

/*
 * Como o 2x2 nao possui centros, qualquer uma das 24 orientacoes globais
 * de uma solucao com cada face uniforme e aceita. Em vez de escolher uma
 * arbitrariamente, montamos todas as metas fisicamente compativeis.
 */
static void gerarMetasSolucao(const int mascaras[8])
{
    int usado[7] = {0};
    int coresFace[6];
    numMetasSolucao = 0;

    /* 6! possibilidades, poucas o suficiente para verificar diretamente. */
    for (int a = 1; a <= 6; a++)
    {
        if (usado[a]) continue;
        usado[a] = 1; coresFace[0] = a;

        for (int b = 1; b <= 6; b++)
        {
            if (usado[b]) continue;
            usado[b] = 1; coresFace[1] = b;

            for (int c = 1; c <= 6; c++)
            {
                if (usado[c]) continue;
                usado[c] = 1; coresFace[2] = c;

                for (int d = 1; d <= 6; d++)
                {
                    if (usado[d]) continue;
                    usado[d] = 1; coresFace[3] = d;

                    for (int e = 1; e <= 6; e++)
                    {
                        if (usado[e]) continue;
                        usado[e] = 1; coresFace[4] = e;

                        for (int g = 1; g <= 6; g++)
                        {
                            if (usado[g]) continue;
                            usado[g] = 1; coresFace[5] = g;

                            int valido = 1;
                            unsigned char meta[8];

                            for (int pos = 0; pos < 8; pos++)
                            {
                                int mascara = mascaraCantoMeta(coresFace, pos);
                                int id = idCantoPorMascara(mascaras, mascara);

                                if (id == -1)
                                {
                                    valido = 0;
                                    break;
                                }

                                meta[pos] = (unsigned char)id;
                            }

                            if (valido)
                            {
                                int idsUsados[8] = {0};
                                for (int pos = 0; pos < 8; pos++)
                                {
                                    if (idsUsados[meta[pos]])
                                    {
                                        valido = 0;
                                        break;
                                    }
                                    idsUsados[meta[pos]] = 1;
                                }
                            }

                            if (valido)
                            {
                                int diferente = 0;

                                for (int i = 0; i < numMetasSolucao; i++)
                                {
                                    if (memcmp(metasSolucao[i], meta, 8) == 0)
                                    {
                                        diferente = 1;
                                        break;
                                    }
                                }

                                if (!diferente && numMetasSolucao < MAX_METAS_SOLUCAO)
                                {
                                    memcpy(metasSolucao[numMetasSolucao], meta, 8);
                                    memcpy(coresMetasSolucao[numMetasSolucao], coresFace, sizeof(coresFace));
                                    numMetasSolucao++;
                                }
                            }

                            usado[g] = 0;
                        }

                        usado[e] = 0;
                    }

                    usado[d] = 0;
                }

                usado[c] = 0;
            }

            usado[b] = 0;
        }

        usado[a] = 0;
    }
}

/*
 * Extrai uma identificacao fixa dos 8 cubinhos a partir das mascaras de
 * cores. A ordem dos IDs fica presa ao estado inicial do solver.
 */
static int idsCantosAtualSolucao(unsigned char ids[8])
{
    for (int pos = 0; pos < 8; pos++)
    {
        int mascara = mascaraCantoAtual(pos);

        if (mascara < 0 || mascara >= 64)
            return 0;

        int id = idCantoPorMascaraSolucao[mascara];
        if (id < 0 || id >= 8)
            return 0;

        ids[pos] = (unsigned char)id;
    }

    return 1;
}

static int distanciaPermutacaoParaMeta(const unsigned char atual[8],
                                       const unsigned char meta[8])
{
    unsigned char posicaoMeta[8];

    for (int i = 0; i < 8; i++)
        posicaoMeta[meta[i]] = (unsigned char)i;

    unsigned char relativa[8];

    for (int pos = 0; pos < 8; pos++)
        relativa[pos] = posicaoMeta[atual[pos]];

    return distPermCanto[rankPermutacao8(relativa)];
}


static int orientacaoPdbDaFace(int face)
{
    if (face == U || face == D) return 0;
    if (face == F || face == B) return 1;
    return 2; /* L/R */
}

static Vetor3 normalPdbDoEstado(Vetor3 pos, int orientacao)
{
    switch (orientacao)
    {
        case 0: return vetor(0, pos.y, 0);
        case 1: return vetor(0, 0, pos.z);
        default: return vetor(pos.x, 0, 0);
    }
}

static int codificarPdbCanto(const unsigned char pos[4],
                             const unsigned char ori[4])
{
    int posCode = (((int)pos[0] * 8 + pos[1]) * 8 + pos[2]) * 8 + pos[3];
    int oriCode = (((int)ori[0] * 3 + ori[1]) * 3 + ori[2]) * 3 + ori[3];
    return posCode * 81 + oriCode;
}

static void decodificarPdbCanto(int codigo,
                                unsigned char pos[4],
                                unsigned char ori[4])
{
    int oriCode = codigo % 81;
    int posCode = codigo / 81;

    for (int i = 3; i >= 0; i--)
    {
        ori[i] = (unsigned char)(oriCode % 3);
        oriCode /= 3;
    }

    for (int i = 3; i >= 0; i--)
    {
        pos[i] = (unsigned char)(posCode % 8);
        posCode /= 8;
    }
}

static void aplicarMovimentoPdb(int movimento,
                                const unsigned char entradaPos[4],
                                const unsigned char entradaOri[4],
                                unsigned char saidaPos[4],
                                unsigned char saidaOri[4])
{
    Vetor3 eixo = eixoDaFaixa(movimentosSolucao[movimento].face,
                               movimentosSolucao[movimento].faixa);
    int sinal = sinalHorarioDaFaixa(movimentosSolucao[movimento].face,
                                    movimentosSolucao[movimento].faixa);

    if (!movimentosSolucao[movimento].sentido)
        sinal = -sinal;

    for (int i = 0; i < 4; i++)
    {
        Vetor3 posicao = coordenadasCantos[entradaPos[i]];

        int pertence =
            posicao.x * eixo.x +
            posicao.y * eixo.y +
            posicao.z * eixo.z == 1;

        if (!pertence)
        {
            saidaPos[i] = entradaPos[i];
            saidaOri[i] = entradaOri[i];
            continue;
        }

        Vetor3 normal = normalPdbDoEstado(posicao, entradaOri[i]);
        Vetor3 novaPosicao = rotacionar90(posicao, eixo, sinal);
        Vetor3 novaNormal = rotacionar90(normal, eixo, sinal);
        int novoCanto = indiceCantoPorCoordenada(novaPosicao);

        saidaPos[i] = (unsigned char)novoCanto;

        if (novaNormal.y != 0)
            saidaOri[i] = 0;
        else if (novaNormal.z != 0)
            saidaOri[i] = 1;
        else
            saidaOri[i] = 2;
    }
}

static void construirPdbCanto(unsigned char *pdb,
                              int posicaoInicial)
{
    memset(pdb, 255, TAM_PDB_CANTO);

    uint32_t *fila = (uint32_t *)malloc(
        TAM_PDB_CANTO * sizeof(uint32_t));

    if (fila == NULL)
        return;

    unsigned char pos[4] = {
        (unsigned char)posicaoInicial,
        (unsigned char)(posicaoInicial + 1),
        (unsigned char)(posicaoInicial + 2),
        (unsigned char)(posicaoInicial + 3)
    };
    unsigned char ori[4] = {0, 0, 0, 0};

    int inicial = codificarPdbCanto(pos, ori);
    fila[0] = (uint32_t)inicial;
    int inicio = 0;
    int fim = 1;
    pdb[inicial] = 0;

    while (inicio < fim)
    {
        int atualCodigo = (int)fila[inicio++];
        unsigned char atualPos[4], atualOri[4];
        decodificarPdbCanto(atualCodigo, atualPos, atualOri);

        for (int m = 0; m < NUM_MOVIMENTOS_SOLUCAO; m++)
        {
            unsigned char novaPos[4], novaOri[4];
            aplicarMovimentoPdb(m, atualPos, atualOri,
                                novaPos, novaOri);

            int novoCodigo = codificarPdbCanto(novaPos, novaOri);

            if (pdb[novoCodigo] == 255)
            {
                pdb[novoCodigo] = (unsigned char)(pdb[atualCodigo] + 1);
                fila[fim++] = (uint32_t)novoCodigo;
            }
        }
    }

    free(fila);
}

static void prepararPdbCantoSolucao(void)
{
    if (pdbPreparadoSolucao)
        return;

    construirPdbCanto(pdbCantoA, 0);
    construirPdbCanto(pdbCantoB, 4);
    pdbPreparadoSolucao = 1;
}

static int estadoPdbParaMeta(const unsigned char ids[8], int metaIndex,
                             int inicioGrupo,
                             unsigned char pos[4],
                             unsigned char ori[4])
{
    for (int i = 0; i < 4; i++)
    {
        int posMeta = inicioGrupo + i;
        int idCanto = metasSolucao[metaIndex][posMeta];
        int posAtual = -1;

        for (int p = 0; p < 8; p++)
        {
            if (ids[p] == idCanto)
            {
                posAtual = p;
                break;
            }
        }

        if (posAtual == -1)
            return 0;

        pos[i] = (unsigned char)posAtual;

        int corUD = (posMeta < 4)
            ? coresMetasSolucao[metaIndex][U]
            : coresMetasSolucao[metaIndex][D];

        int encontrado = 0;

        for (int k = 0; k < 3; k++)
        {
            int indice = cantosSticker[posAtual][k];
            int face = indice / STICKERS_POR_FACE;
            int sticker = indice % STICKERS_POR_FACE;

            if (cube[face][sticker] == corUD)
            {
                ori[i] = (unsigned char)orientacaoPdbDaFace(face);
                encontrado = 1;
                break;
            }
        }

        if (!encontrado)
            return 0;
    }

    return 1;
}

static int heuristicaPdbSolucao(const unsigned char ids[8])
{
    int melhor = 255;

    for (int meta = 0; meta < numMetasSolucao; meta++)
    {
        unsigned char posA[4], oriA[4];
        unsigned char posB[4], oriB[4];

        if (!estadoPdbParaMeta(ids, meta, 0, posA, oriA) ||
            !estadoPdbParaMeta(ids, meta, 4, posB, oriB))
            continue;

        int da = pdbCantoA[codificarPdbCanto(posA, oriA)];
        int db = pdbCantoB[codificarPdbCanto(posB, oriB)];
        int d = (da > db) ? da : db;

        if (d < melhor)
            melhor = d;
    }

    return (melhor == 255) ? 0 : melhor;
}

/* Heuristica admissivel: distancia minima da permutacao dos cantos,
   ignorando sua orientacao. Ignorar orientacao nunca aumenta a distancia. */
static int heuristicaSolucao(const unsigned char ids[8])
{
    int hPerm = 255;
    int hPdb = heuristicaPdbSolucao(ids);

    for (int i = 0; i < numMetasSolucao; i++)
    {
        int d = distanciaPermutacaoParaMeta(ids, metasSolucao[i]);
        if (d < hPerm)
            hPerm = d;
    }

    if (hPerm == 255)
        hPerm = 0;

    return (hPdb > hPerm) ? hPdb : hPerm;
}

static int faceAfetadaPorMovimento(int movimento, int face)
{
    for (int p = 0; p < STICKERS_POR_FACE; p++)
    {
        int origem = indiceStickerLinear(face, p);
        if (permStickerSolucao[movimento][origem] != (unsigned char)origem)
            return 1;
    }

    return 0;
}

static void ordenarMovimentosSolucao(int ordem[NUM_MOVIMENTOS_SOLUCAO],
                                     int ultimo)
{
    for (int i = 0; i < NUM_MOVIMENTOS_SOLUCAO; i++)
        ordem[i] = i;

    /* Preferimos primeiro movimentos que alteram o lado escolhido pelo
       usuario. Isso implementa a ideia de trabalhar primeiro esse lado,
       sem proibir movimentos necessarios para resolver os demais. */
    for (int i = 0; i < NUM_MOVIMENTOS_SOLUCAO - 1; i++)
    {
        for (int j = i + 1; j < NUM_MOVIMENTOS_SOLUCAO; j++)
        {
            int pi = faceAfetadaPorMovimento(ordem[i], ladoPrioritarioSolucao);
            int pj = faceAfetadaPorMovimento(ordem[j], ladoPrioritarioSolucao);

            if (pj > pi)
            {
                int aux = ordem[i];
                ordem[i] = ordem[j];
                ordem[j] = aux;
            }
        }
    }

    (void)ultimo;
}

static void aplicarMovimentoRapidoSolucao(int movimento)
{
    int antigo[NUM_FACES][STICKERS_POR_FACE];

    memcpy(antigo, cube, sizeof(cube));

    for (int origem = 0; origem < 24; origem++)
    {
        int of = origem / STICKERS_POR_FACE;
        int op = origem % STICKERS_POR_FACE;
        int destino = permStickerSolucao[movimento][origem];
        int df = destino / STICKERS_POR_FACE;
        int dp = destino % STICKERS_POR_FACE;

        cube[df][dp] = antigo[of][op];
    }
}

static int mesmoTrechoDeMovimento(int atual, int anterior, int anterior2)
{
    if (anterior < 0)
        return 0;

    int camadaAtual = atual / 2;
    int camadaAnterior = anterior / 2;

    /* Movimento inverso imediatamente depois nao acrescenta nada. */
    if (camadaAtual == camadaAnterior && (atual ^ 1) == anterior)
        return 1;

    /* Tres quartos de volta na mesma camada equivalem ao inverso. */
    if (anterior2 >= 0 &&
        camadaAtual == camadaAnterior &&
        atual == anterior &&
        anterior == anterior2)
        return 1;

    return 0;
}

static int dfsSolucao(unsigned char ids[8], int profundidade,
                      int limite, int ultimo, int penultimo)
{
    nosExploradosSolucao++;

    if (estadoResolvido())
        return 1;

    int h = heuristicaSolucao(ids);
    if (profundidade + h > limite)
        return 0;

    if (profundidade >= limite)
        return 0;

    uint64_t hash = hashEstadoSolucao();
    unsigned int base = (unsigned int)((hash ^ (hash >> 32)) &
                                       (TAM_HASH_SOLUCAO - 1));

    for (unsigned int tentativa = 0; tentativa < 8; tentativa++)
    {
        EntradaHashSolucao *entrada =
            &tabelaHashSolucao[(base + tentativa) & (TAM_HASH_SOLUCAO - 1)];

        if (!entrada->usado)
            break;

        if (entrada->hash == hash &&
            entrada->ultimo == ultimo &&
            entrada->profundidade <= profundidade)
            return 0;
    }

    for (unsigned int tentativa = 0; tentativa < 8; tentativa++)
    {
        EntradaHashSolucao *entrada =
            &tabelaHashSolucao[(base + tentativa) & (TAM_HASH_SOLUCAO - 1)];

        if (!entrada->usado || entrada->profundidade > profundidade)
        {
            entrada->usado = 1;
            entrada->hash = hash;
            entrada->profundidade = (unsigned char)profundidade;
            entrada->ultimo = (signed char)ultimo;
            break;
        }
    }

    int ordem[NUM_MOVIMENTOS_SOLUCAO];
    ordenarMovimentosSolucao(ordem, ultimo);

    for (int i = 0; i < NUM_MOVIMENTOS_SOLUCAO; i++)
    {
        int movimento = ordem[i];

        if (mesmoTrechoDeMovimento(movimento, ultimo, penultimo))
            continue;

        aplicarMovimentoRapidoSolucao(movimento);

        unsigned char novoIds[8];
        for (int c = 0; c < 8; c++)
            novoIds[permCantoSolucao[movimento][c]] = ids[c];

        caminhoSolucao[profundidade] = movimento;

        if (dfsSolucao(novoIds, profundidade + 1, limite,
                       movimento, ultimo))
            return 1;

        aplicarMovimentoRapidoSolucao(movimento ^ 1);
    }

    return 0;
}

static int estadoFisicoValidoParaSolucao(int mascaras[8])
{
    if (!prepararCantosStickerSolucao())
    {
        printf("\nErro: nao foi possivel identificar os 8 cantos do cubo.\n");
        return 0;
    }

    for (int c = 0; c < 8; c++)
    {
        mascaras[c] = mascaraCantoAtual(c);

        if (mascaras[c] < 0)
        {
            printf("\nErro: foi encontrada uma cor invalida em um canto do cubo.\n");
            return 0;
        }
    }

    /* Cada cubinho de canto precisa ser formado por uma combinacao de
       tres cores diferente dos outros sete cubinhos. Apenas ter 4
       adesivos de cada cor nao garante que o estado seja fisicamente
       alcancavel. */
    for (int i = 0; i < 8; i++)
    {
        for (int j = i + 1; j < 8; j++)
        {
            if (mascaras[i] == mascaras[j])
            {
                printf("\nEstado do cubo invalido para movimentos fisicos.\n");
                printf("Os cantos %d e %d possuem a mesma combinacao de cores.\n",
                       i + 1, j + 1);
                printf("Ter 4 adesivos de cada cor, sozinho, nao garante um cubo solucionavel.\n");
                return 0;
            }
        }
    }

    return 1;
}

static int prepararSolucionadorProfundidade(void)
{
    if (solverPreparado)
        return 1;

    prepararPermutacoesStickerSolucao();
    prepararPermutacoesCantoSolucao();
    prepararDistanciasCantoSolucao();

    int mascaras[8];
    if (!estadoFisicoValidoParaSolucao(mascaras))
        return 0;

    for (int i = 0; i < 64; i++)
        idCantoPorMascaraSolucao[i] = -1;

    for (int pos = 0; pos < 8; pos++)
        idCantoPorMascaraSolucao[mascaras[pos]] = pos;

    gerarMetasSolucao(mascaras);

    if (numMetasSolucao == 0)
    {
        printf("\nErro: nao foi encontrada nenhuma configuracao final compativel com este cubo.\n");
        return 0;
    }

    prepararPdbCantoSolucao();

    tabelaHashSolucao = (EntradaHashSolucao *)
        calloc(TAM_HASH_SOLUCAO, sizeof(EntradaHashSolucao));

    if (tabelaHashSolucao == NULL)
    {
        printf("\nErro: nao foi possivel reservar memoria para a busca.\n");
        return 0;
    }

    solverPreparado = 1;
    return 1;
}

/*
 * Executa a busca em profundidade.
 *
 * ladoInicial = uma das faces U,D,F,B,L,R. Ele define a prioridade da
 * primeira etapa da busca, mas nao bloqueia movimentos que sejam necessarios
 * para concluir os outros lados.
 *
 * Retorno:
 *   > 0 = numero de movimentos encontrados;
 *   -1  = cubo/solucionador invalido;
 *    0  = nenhuma solucao encontrada dentro do limite definido.
 */
int resolverEmProfundidade(int ladoInicial)
{
    double inicioTotal;
    double inicioBusca;

    if (ladoInicial < 0 || ladoInicial >= NUM_FACES)
        return -1;

    inicioTotal = tempoAtualSolucao();

    solverPreparado = 0;
    if (tabelaHashSolucao != NULL)
    {
        free(tabelaHashSolucao);
        tabelaHashSolucao = NULL;
    }

    ladoPrioritarioSolucao = ladoInicial;

    if (!prepararSolucionadorProfundidade())
        return -1;

    unsigned char ids[8];
    if (!idsCantosAtualSolucao(ids))
        return -1;

    nosExploradosSolucao = 0;
    inicioBusca = tempoAtualSolucao();

    for (int limite = 0; limite <= MAX_PROFUNDIDADE_SOLUCAO; limite++)
    {
        memset(tabelaHashSolucao, 0,
               TAM_HASH_SOLUCAO * sizeof(EntradaHashSolucao));

        if (dfsSolucao(ids, 0, limite, -1, -1))
        {
            double fimBusca = tempoAtualSolucao();
            double fimTotal = fimBusca;

            printf("\nSolucao encontrada por busca em profundidade!\n");
            printf("Lado priorizado: %c\n", letraFaceSolucao(ladoPrioritarioSolucao));
            printf("Profundidade: %d movimento(s)\n", limite);
            printf("Nos explorados: %llu\n", nosExploradosSolucao);
            printf("Tempo da busca: %.6f segundos\n", fimBusca - inicioBusca);
            printf("Tempo total: %.6f segundos\n", fimTotal - inicioTotal);

            if (limite == 0)
            {
                printf("Ordem de solucao: cubo ja estava resolvido.\n");
            }
            else
            {
                printf("Ordem de solucao: \n");

                for (int i = 0; i < limite; i++)
                {
                    MovimentoSolucao m = movimentosSolucao[caminhoSolucao[i]];
                    printf("%d. %c + %s + %s",
                           i + 1,
                           letraFaceSolucao(m.face),
                           nomeFaixaSolucao(m.faixa),
                           m.sentido ? "Horario" : "Anti-horario");

                    if (i < limite - 1)
                        printf(" -> \n");
                }

                printf("\n");
            }

            return limite;
        }
    }

    printf("\nNao foi encontrada uma solucao ate a profundidade %d.\n",
           MAX_PROFUNDIDADE_SOLUCAO);
    printf("Nos explorados: %llu\n", nosExploradosSolucao);
    printf("Tempo da busca: %.6f segundos\n", tempoAtualSolucao() - inicioBusca);
    printf("Tempo total: %.6f segundos\n", tempoAtualSolucao() - inicioTotal);
    return 0;
}



/* ============================================================
 * BUSCA DE SOLUCAO POR CAMADAS
 * ============================================================
 *
 * A ideia e dividir a resolucao em duas etapas, sem usar pontuacao:
 *
 *   1. buscar em profundidade uma configuracao em que a face U esteja
 *      uniforme, representando a camada superior pronta;
 *   2. a partir desse estado, executar uma busca de profundidade normal
 *      ate chegar ao cubo completamente resolvido.
 *
 * A primeira etapa nao calcula nota, custo ou distancia. Ela apenas
 * verifica se o objetivo intermediario foi atingido.
 */

#define MAX_PROFUNDIDADE_CAMADAS 8

static int caminhoCamadas[MAX_PROFUNDIDADE_CAMADAS];
static unsigned long long nosCamadaExplorados = 0;

static int faceUniformeCamadas(int face)
{
    int cor = cube[face][0];

    for (int p = 1; p < STICKERS_POR_FACE; p++)
    {
        if (cube[face][p] != cor)
            return 0;
    }

    return 1;
}

/* Busca cega da primeira etapa: nao existe pontuacao nem heuristica. */
static int dfsCamadaSuperior(int profundidade, int limite,
                             int ultimo, int penultimo)
{
    nosCamadaExplorados++;

    if (faceUniformeCamadas(U))
        return 1;

    if (profundidade >= limite)
        return 0;

    for (int movimento = 0; movimento < NUM_MOVIMENTOS_SOLUCAO; movimento++)
    {
        if (mesmoTrechoDeMovimento(movimento, ultimo, penultimo))
            continue;

        aplicarMovimentoRapidoSolucao(movimento);
        caminhoCamadas[profundidade] = movimento;

        if (dfsCamadaSuperior(profundidade + 1, limite,
                              movimento, ultimo))
            return 1;

        aplicarMovimentoRapidoSolucao(movimento ^ 1);
    }

    return 0;
}

/*
 * Executa a segunda etapa do metodo por camadas sem imprimir o relatorio
 * da DFS publica. Assim conseguimos montar um unico relatorio contendo
 * as duas fases.
 */
static int executarProfundidadeInternaSemRelatorio(
    unsigned long long *nosExplorados,
    double *tempoBusca)
{
    double inicio = tempoAtualSolucao();

    solverPreparado = 0;

    if (tabelaHashSolucao != NULL)
    {
        free(tabelaHashSolucao);
        tabelaHashSolucao = NULL;
    }

    ladoPrioritarioSolucao = U;

    if (!prepararSolucionadorProfundidade())
        return -1;

    unsigned char ids[8];
    if (!idsCantosAtualSolucao(ids))
        return -1;

    nosExploradosSolucao = 0;

    for (int limite = 0; limite <= MAX_PROFUNDIDADE_SOLUCAO; limite++)
    {
        memset(tabelaHashSolucao, 0,
               TAM_HASH_SOLUCAO * sizeof(EntradaHashSolucao));

        if (dfsSolucao(ids, 0, limite, -1, -1))
        {
            *nosExplorados = nosExploradosSolucao;
            *tempoBusca = tempoAtualSolucao() - inicio;
            return limite;
        }
    }

    *nosExplorados = nosExploradosSolucao;
    *tempoBusca = tempoAtualSolucao() - inicio;
    return 0;
}

int resolverPorCamadas(void)
{
    double inicioTotal = tempoAtualSolucao();
    double inicioPrimeira;
    double tempoPrimeira = 0.0;
    double tempoSegunda = 0.0;
    unsigned long long nosPrimeira = 0;
    unsigned long long nosSegunda = 0;
    int profundidadePrimeira = -1;
    int profundidadeSegunda = -1;

    prepararPermutacoesStickerSolucao();

    int mascaras[8];
    if (!estadoFisicoValidoParaSolucao(mascaras))
        return -1;

    inicioPrimeira = tempoAtualSolucao();
    nosCamadaExplorados = 0;

    if (faceUniformeCamadas(U))
    {
        profundidadePrimeira = 0;
    }
    else
    {
        for (int limite = 1; limite <= MAX_PROFUNDIDADE_CAMADAS; limite++)
        {
            memset(caminhoCamadas, -1, sizeof(caminhoCamadas));

            if (dfsCamadaSuperior(0, limite, -1, -1))
            {
                profundidadePrimeira = limite;
                break;
            }
        }
    }

    tempoPrimeira = tempoAtualSolucao() - inicioPrimeira;
    nosPrimeira = nosCamadaExplorados;

    if (profundidadePrimeira == -1)
    {
        printf("\nNao foi possivel completar a primeira etapa (camada superior)\n");
        printf("ate a profundidade %d.\n", MAX_PROFUNDIDADE_CAMADAS);
        printf("Nos explorados na primeira etapa: %llu\n", nosPrimeira);
        printf("Tempo da primeira etapa: %.6f segundos\n", tempoPrimeira);
        printf("Tempo total: %.6f segundos\n",
               tempoAtualSolucao() - inicioTotal);
        return 0;
    }

    /* Apos a DFS de busca da camada, o cubo esta exatamente no estado
       intermediario encontrado, pois os movimentos do caminho de sucesso
       nao sao desfeitos. */
    int caminhoPrimeira[MAX_PROFUNDIDADE_CAMADAS];
    for (int i = 0; i < profundidadePrimeira; i++)
        caminhoPrimeira[i] = caminhoCamadas[i];

    if (estadoResolvido())
    {
        printf("\nSolucao encontrada por busca por camadas!\n");
        printf("Primeira etapa (camada superior): %d movimento(s)\n",
               profundidadePrimeira);
        printf("Segunda etapa: 0 movimento(s)\n");
        printf("Total: %d movimento(s)\n", profundidadePrimeira);
        printf("Nos explorados na primeira etapa: %llu\n", nosPrimeira);
        printf("Tempo da primeira etapa: %.6f segundos\n", tempoPrimeira);
        printf("Tempo da segunda etapa: 0.000000 segundos\n");
        printf("Tempo total: %.6f segundos\n",
               tempoAtualSolucao() - inicioTotal);
        printf("Ordem de solucao:\n");

        if (profundidadePrimeira == 0)
            printf("Cubo ja estava resolvido.\n");
        else
        {
            for (int i = 0; i < profundidadePrimeira; i++)
            {
                MovimentoSolucao m = movimentosSolucao[caminhoPrimeira[i]];
                printf("%d. %c + %s + %s\n",
                       i + 1,
                       letraFaceSolucao(m.face),
                       nomeFaixaSolucao(m.faixa),
                       m.sentido ? "Horario" : "Anti-horario");
            }
        }

        if (tabelaHashSolucao != NULL)
        {
            free(tabelaHashSolucao);
            tabelaHashSolucao = NULL;
        }
        solverPreparado = 0;
        return profundidadePrimeira;
    }

    profundidadeSegunda = executarProfundidadeInternaSemRelatorio(
        &nosSegunda, &tempoSegunda);

    if (profundidadeSegunda <= 0 && !estadoResolvido())
    {
        printf("\nNao foi encontrada a solucao na segunda etapa por profundidade.\n");
        printf("Primeira etapa (camada superior): %d movimento(s)\n",
               profundidadePrimeira);
        printf("Nos explorados na primeira etapa: %llu\n", nosPrimeira);
        printf("Tempo da primeira etapa: %.6f segundos\n", tempoPrimeira);
        printf("Nos explorados na segunda etapa: %llu\n", nosSegunda);
        printf("Tempo da segunda etapa: %.6f segundos\n", tempoSegunda);
        printf("Tempo total: %.6f segundos\n",
               tempoAtualSolucao() - inicioTotal);

        if (tabelaHashSolucao != NULL)
        {
            free(tabelaHashSolucao);
            tabelaHashSolucao = NULL;
        }
        solverPreparado = 0;
        return 0;
    }

    printf("\nSolucao encontrada por busca por camadas!\n");
    printf("Primeira etapa (camada superior): %d movimento(s)\n",
           profundidadePrimeira);
    printf("Segunda etapa: %d movimento(s)\n", profundidadeSegunda);
    printf("Total: %d movimento(s)\n",
           profundidadePrimeira + profundidadeSegunda);
    printf("Nos explorados na primeira etapa: %llu\n", nosPrimeira);
    printf("Nos explorados na segunda etapa: %llu\n", nosSegunda);
    printf("Tempo da primeira etapa: %.6f segundos\n", tempoPrimeira);
    printf("Tempo da segunda etapa: %.6f segundos\n", tempoSegunda);
    printf("Tempo total: %.6f segundos\n",
           tempoAtualSolucao() - inicioTotal);

    printf("\nOrdem de solucao:\n");

    int numero = 1;

    for (int i = 0; i < profundidadePrimeira; i++, numero++)
    {
        MovimentoSolucao m = movimentosSolucao[caminhoPrimeira[i]];
        printf("%d. %c + %s + %s",
               numero,
               letraFaceSolucao(m.face),
               nomeFaixaSolucao(m.faixa),
               m.sentido ? "Horario" : "Anti-horario");
        printf(" ->\n");
    }

    for (int i = 0; i < profundidadeSegunda; i++, numero++)
    {
        MovimentoSolucao m = movimentosSolucao[caminhoSolucao[i]];
        printf("%d. %c + %s + %s",
               numero,
               letraFaceSolucao(m.face),
               nomeFaixaSolucao(m.faixa),
               m.sentido ? "Horario" : "Anti-horario");

        if (i < profundidadeSegunda - 1)
            printf(" ->\n");
        else
            printf("\n");
    }

    if (profundidadePrimeira == 0 && profundidadeSegunda == 0)
        printf("Cubo ja estava resolvido.\n");

    if (tabelaHashSolucao != NULL)
    {
        free(tabelaHashSolucao);
        tabelaHashSolucao = NULL;
    }
    solverPreparado = 0;

    return profundidadePrimeira + profundidadeSegunda;
}



/* ============================================================
 * BUSCA DE SOLUCAO EM LARGURA (BFS)
 * ============================================================
 *
 * A busca em largura explora todos os estados a uma determinada
 * profundidade antes de passar para a proxima. Ao encontrar um
 * estado resolvido, a primeira solucao encontrada tem o menor
 * numero de movimentos dentro deste conjunto de movimentos.
 *
 * A BFS usa os mesmos 12 movimentos fisicos canonicos da DFS e a
 * mesma tabela de permutacoes de stickers. A fila B_FILA guarda
 * apenas o indice do no a ser expandido.
 */

typedef struct
{
    uint64_t baixo;   /* 21 primeiros stickers, 3 bits cada. */
    uint32_t alto;    /* 3 ultimos stickers, 3 bits cada. */
} EstadoCompactoBFS;

typedef struct
{
    EstadoCompactoBFS estado;
    int pai;
    unsigned char movimento;
    unsigned char profundidade;
} NoBFS;

typedef struct
{
    uint64_t hash;
    int indice;
} EntradaHashBFS;

#define MAX_NOS_BFS 3000000u
#define TAM_HASH_BFS (1u << 22)
#define LIMITE_CARGA_BFS ((TAM_HASH_BFS * 7u) / 10u)
#define MAX_PROFUNDIDADE_BFS 255

static EstadoCompactoBFS compactarEstadoBFS(const unsigned char estado[24])
{
    EstadoCompactoBFS compacto;
    compacto.baixo = 0;
    compacto.alto = 0;

    for (int i = 0; i < 21; i++)
    {
        uint64_t valor = (uint64_t)(estado[i] - 1) & 7u;
        compacto.baixo |= valor << (3 * i);
    }

    for (int i = 21; i < 24; i++)
    {
        uint32_t valor = (uint32_t)(estado[i] - 1) & 7u;
        compacto.alto |= valor << (3 * (i - 21));
    }

    return compacto;
}

static void descompactarEstadoBFS(EstadoCompactoBFS compacto,
                                  unsigned char estado[24])
{
    for (int i = 0; i < 21; i++)
        estado[i] = (unsigned char)(((compacto.baixo >> (3 * i)) & 7u) + 1u);

    for (int i = 21; i < 24; i++)
        estado[i] = (unsigned char)(((compacto.alto >> (3 * (i - 21))) & 7u) + 1u);
}

static int estadosCompactosIguaisBFS(EstadoCompactoBFS a,
                                      EstadoCompactoBFS b)
{
    return a.baixo == b.baixo && a.alto == b.alto;
}

static uint64_t hashEstadoCompactoBFS(EstadoCompactoBFS estado)
{
    uint64_t h = UINT64_C(1469598103934665603);

    for (int i = 0; i < 8; i++)
    {
        h ^= (estado.baixo >> (8 * i)) & UINT64_C(0xff);
        h *= UINT64_C(1099511628211);
    }

    for (int i = 0; i < 4; i++)
    {
        h ^= (estado.alto >> (8 * i)) & UINT64_C(0xff);
        h *= UINT64_C(1099511628211);
    }

    return h;
}

static int buscarEstadoNaTabelaBFS(const EntradaHashBFS tabela[],
                                   const NoBFS nos[],
                                   EstadoCompactoBFS estado,
                                   uint64_t hash)
{
    unsigned int base = (unsigned int)(hash & (TAM_HASH_BFS - 1u));

    for (unsigned int tentativa = 0; tentativa < TAM_HASH_BFS; tentativa++)
    {
        const EntradaHashBFS *entrada =
            &tabela[(base + tentativa) & (TAM_HASH_BFS - 1u)];

        if (entrada->indice == -1)
            return -1;

        if (entrada->hash == hash &&
            estadosCompactosIguaisBFS(nos[entrada->indice].estado, estado))
            return entrada->indice;
    }

    return -1;
}

static int inserirEstadoNaTabelaBFS(EntradaHashBFS tabela[],
                                    EstadoCompactoBFS estado,
                                    uint64_t hash,
                                    int indice,
                                    const NoBFS nos[])
{
    unsigned int base = (unsigned int)(hash & (TAM_HASH_BFS - 1u));

    for (unsigned int tentativa = 0; tentativa < TAM_HASH_BFS; tentativa++)
    {
        EntradaHashBFS *entrada =
            &tabela[(base + tentativa) & (TAM_HASH_BFS - 1u)];

        if (entrada->indice == -1)
        {
            entrada->hash = hash;
            entrada->indice = indice;
            return 1;
        }

        if (entrada->hash == hash &&
            estadosCompactosIguaisBFS(nos[entrada->indice].estado, estado))
            return 0;
    }

    return -1;
}

static int estadoResolvidoBFS(EstadoCompactoBFS compacto)
{
    unsigned char estado[24];
    descompactarEstadoBFS(compacto, estado);

    for (int face = 0; face < NUM_FACES; face++)
    {
        unsigned char cor = estado[face * STICKERS_POR_FACE];

        for (int p = 1; p < STICKERS_POR_FACE; p++)
        {
            if (estado[face * STICKERS_POR_FACE + p] != cor)
                return 0;
        }
    }

    return 1;
}

static EstadoCompactoBFS aplicarMovimentoEstadoBFS(
    EstadoCompactoBFS estadoCompacto,
    int movimento)
{
    unsigned char estado[24];
    unsigned char novoEstado[24];

    descompactarEstadoBFS(estadoCompacto, estado);

    for (int origem = 0; origem < 24; origem++)
        novoEstado[permStickerSolucao[movimento][origem]] = estado[origem];

    return compactarEstadoBFS(novoEstado);
}

static void copiarCubeParaEstadoBFS(unsigned char estado[24])
{
    for (int face = 0; face < NUM_FACES; face++)
    {
        for (int pos = 0; pos < STICKERS_POR_FACE; pos++)
        {
            int indice = face * STICKERS_POR_FACE + pos;
            estado[indice] = (unsigned char)cube[face][pos];
        }
    }
}

static void aplicarEstadoAoCubeBFS(EstadoCompactoBFS compacto)
{
    unsigned char estado[24];
    descompactarEstadoBFS(compacto, estado);

    for (int face = 0; face < NUM_FACES; face++)
    {
        for (int pos = 0; pos < STICKERS_POR_FACE; pos++)
        {
            int indice = face * STICKERS_POR_FACE + pos;
            cube[face][pos] = estado[indice];
        }
    }
}

static void imprimirMovimentoBFS(int movimento, int numero)
{
    MovimentoSolucao m = movimentosSolucao[movimento];

    printf("%d. %c + %s + %s",
           numero,
           letraFaceSolucao(m.face),
           nomeFaixaSolucao(m.faixa),
           m.sentido ? "Horario" : "Anti-horario");
}

/*
 * Executa a busca em largura.
 *
 * Como a BFS nao precisa escolher uma face prioritaria, ela percorre
 * normalmente todos os movimentos canonicos em cada nivel.
 *
 * Retorno:
 *   > 0 = numero de movimentos encontrados;
 *   -1  = estado invalido ou erro de memoria;
 *    0  = nao encontrou dentro dos limites configurados.
 */
int resolverEmLargura(void)
{
    double inicio = tempoAtualSolucao();
    uint64_t estadosVisitados = 0;
    uint64_t nosExpandidos = 0;
    int resultado = -1;

    /* As permutacoes usadas pela BFS sao as mesmas da DFS. */
    prepararPermutacoesStickerSolucao();

    int mascaras[8];
    if (!estadoFisicoValidoParaSolucao(mascaras))
        return -1;

    unsigned char estadoInicial[24];
    copiarCubeParaEstadoBFS(estadoInicial);
    EstadoCompactoBFS inicial = compactarEstadoBFS(estadoInicial);

    NoBFS *nos = (NoBFS *)malloc(sizeof(NoBFS) * MAX_NOS_BFS);
    if (nos == NULL)
    {
        printf("\nErro: nao foi possivel reservar memoria para os nos da busca em largura.\n");
        return -1;
    }

    EntradaHashBFS *tabela =
        (EntradaHashBFS *)malloc(sizeof(EntradaHashBFS) * TAM_HASH_BFS);
    if (tabela == NULL)
    {
        printf("\nErro: nao foi possivel reservar memoria para a tabela de estados da busca em largura.\n");
        free(nos);
        return -1;
    }

    for (size_t i = 0; i < TAM_HASH_BFS; i++)
        tabela[i].indice = -1;

    Fila *fila = CriaFila();
    if (fila == NULL)
    {
        printf("\nErro: nao foi possivel criar a fila da busca em largura.\n");
        free(tabela);
        free(nos);
        return -1;
    }

    unsigned int quantidadeNos = 0;
    nos[0].estado = inicial;
    nos[0].pai = -1;
    nos[0].movimento = 255;
    nos[0].profundidade = 0;

    int insercao = inserirEstadoNaTabelaBFS(
        tabela, inicial, hashEstadoCompactoBFS(inicial), 0, nos);

    if (insercao != 1)
    {
        printf("\nErro interno ao registrar o estado inicial da busca.\n");
        destroiFila(fila);
        free(tabela);
        free(nos);
        return -1;
    }

    quantidadeNos = 1;
    estadosVisitados = 1;
    InsereFila(fila, 0);

    int noObjetivo = -1;

    while (fila->ini != NULL)
    {
        int indiceAtual = removeInicio(fila);
        if (indiceAtual < 0)
            break;

        NoBFS atual = nos[indiceAtual];
        nosExpandidos++;

        if (estadoResolvidoBFS(atual.estado))
        {
            noObjetivo = indiceAtual;
            resultado = (int)atual.profundidade;
            break;
        }

        if (atual.profundidade >= MAX_PROFUNDIDADE_BFS)
            continue;

        for (int movimento = 0; movimento < NUM_MOVIMENTOS_SOLUCAO; movimento++)
        {
            /* Um movimento seguido imediatamente do inverso nao gera um
               estado que a BFS precise visitar de novo. */
            if (atual.movimento < NUM_MOVIMENTOS_SOLUCAO &&
                (movimento ^ 1) == atual.movimento)
                continue;

            EstadoCompactoBFS proximo =
                aplicarMovimentoEstadoBFS(atual.estado, movimento);
            uint64_t hash = hashEstadoCompactoBFS(proximo);

            int existente = buscarEstadoNaTabelaBFS(tabela, nos, proximo, hash);
            if (existente != -1)
                continue;

            if (quantidadeNos >= MAX_NOS_BFS)
            {
                printf("\nLimite de %u estados atingido pela busca em largura.\n",
                       MAX_NOS_BFS);
                printf("A busca foi interrompida para evitar consumo excessivo de memoria.\n");
                resultado = 0;
                goto finaliza_bfs;
            }

            if (estadosVisitados >= LIMITE_CARGA_BFS)
            {
                printf("\nLimite da tabela de estados atingido pela busca em largura.\n");
                printf("A busca foi interrompida para evitar uma tabela excessivamente cheia.\n");
                resultado = 0;
                goto finaliza_bfs;
            }

            int novoIndice = (int)quantidadeNos;
            nos[novoIndice].estado = proximo;
            nos[novoIndice].pai = indiceAtual;
            nos[novoIndice].movimento = (unsigned char)movimento;
            nos[novoIndice].profundidade =
                (unsigned char)(atual.profundidade + 1);

            int inseriu = inserirEstadoNaTabelaBFS(
                tabela, proximo, hash, novoIndice, nos);

            if (inseriu == -1)
            {
                printf("\nA tabela de estados ficou sem espaco para continuar a busca.\n");
                resultado = 0;
                goto finaliza_bfs;
            }

            if (inseriu == 0)
                continue;

            quantidadeNos++;
            estadosVisitados++;
            InsereFila(fila, novoIndice);
        }
    }

    if (noObjetivo == -1 && fila->ini == NULL && resultado == -1)
        resultado = 0;

finaliza_bfs:
    {
        double fim = tempoAtualSolucao();

        if (noObjetivo != -1)
        {
            int profundidade = nos[noObjetivo].profundidade;
            int *caminho = NULL;

            if (profundidade > 0)
                caminho = (int *)malloc(sizeof(int) * profundidade);

            int atual = noObjetivo;
            for (int i = profundidade - 1; i >= 0; i--)
            {
                caminho[i] = nos[atual].movimento;
                atual = nos[atual].pai;
            }

            printf("\nSolucao encontrada por busca em largura!\n");
            printf("Profundidade: %d movimento(s)\n", profundidade);
            printf("Estados visitados: %llu\n",
                   (unsigned long long)estadosVisitados);
            printf("Nos expandidos: %llu\n",
                   (unsigned long long)nosExpandidos);
            printf("Tempo da busca: %.6f segundos\n", fim - inicio);
            printf("Ordem de solucao:\n");

            if (profundidade == 0)
            {
                printf("Cubo ja estava resolvido.\n");
            }
            else
            {
                for (int i = 0; i < profundidade; i++)
                {
                    imprimirMovimentoBFS(caminho[i], i + 1);
                    if (i < profundidade - 1)
                        printf(" ->\n");
                    else
                        printf("\n");
                }
            }

            aplicarEstadoAoCubeBFS(nos[noObjetivo].estado);
            free(caminho);

            destroiFila(fila);
            free(tabela);
            free(nos);

            return profundidade;
        }

        printf("\nNao foi encontrada uma solucao pela busca em largura dentro dos limites configurados.\n");
        printf("Estados visitados: %llu\n",
               (unsigned long long)estadosVisitados);
        printf("Nos expandidos: %llu\n",
               (unsigned long long)nosExpandidos);
        printf("Tempo da busca: %.6f segundos\n", fim - inicio);
    }

    destroiFila(fila);
    free(tabela);
    free(nos);

    return resultado;
}
