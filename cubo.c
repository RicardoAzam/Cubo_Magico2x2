#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cubo.h"

int cube[NUM_FACES][STICKERS_POR_FACE];

/*
 * Estado inicial: guardado logo apos a entrada do cubo, antes do usuario
 * comecar a movimenta-lo.
 *
 * Estado de comparacao: representa o estado que deve ser usado pelos tres
 * metodos de resolucao. Se o usuario fizer algum movimento manual, o estado
 * inicial deixa de ser a referencia e o estado atual passa a ser salvo aqui.
 */
static int estadoInicial[NUM_FACES][STICKERS_POR_FACE];
static int estadoComparacao[NUM_FACES][STICKERS_POR_FACE];
static int possuiEstadoInicial = 0;
static int possuiEstadoComparacao = 0;


const Vetor3 coordenadasCantos[8] =
{
    {-1,  1, -1}, /* 0 = UBL */
    { 1,  1, -1}, /* 1 = UBR */
    {-1,  1,  1}, /* 2 = UFL */
    { 1,  1,  1}, /* 3 = UFR */
    {-1, -1, -1}, /* 4 = DBL */
    { 1, -1, -1}, /* 5 = DBR */
    {-1, -1,  1}, /* 6 = DFL */
    { 1, -1,  1}  /* 7 = DFR */
};


int indiceCantoPorCoordenada(Vetor3 v)
{
    for (int i = 0; i < 8; i++)
        if (vetoresIguais(v, coordenadasCantos[i]))
            return i;
    return -1;
}

int indiceStickerLinear(int face, int pos)
{
    return face * STICKERS_POR_FACE + pos;
}

/* ============================================================
 * REPRESENTACAO GEOMETRICA DOS STICKERS
 * ============================================================
 * Cada sticker possui:
 *   - posicao do seu cubinho (x,y,z), com valores -1 ou +1;
 *   - normal, indicando qual face ele ocupa.
 *
 * Isso permite movimentar exatamente UMA CAMADA do cubo.
 * Um movimento de camada movimenta 4 cubinhos = 12 stickers.
 */

Vetor3 vetor(int x, int y, int z)
{
    Vetor3 v = {x, y, z};
    return v;
}

int vetoresIguais(Vetor3 a, Vetor3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

static Vetor3 somaVetores(Vetor3 a, Vetor3 b)
{
    return vetor(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vetor3 multiplicaVetor(Vetor3 a, int k)
{
    return vetor(a.x * k, a.y * k, a.z * k);
}

static Vetor3 produtoVetorial(Vetor3 a, Vetor3 b)
{
    return vetor(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

static Vetor3 normalDaFace(int face)
{
    switch (face)
    {
        case U: return vetor(0, 1, 0);
        case D: return vetor(0, -1, 0);
        case F: return vetor(0, 0, 1);
        case B: return vetor(0, 0, -1);
        case L: return vetor(-1, 0, 0);
        case R: return vetor(1, 0, 0);
        default: return vetor(0, 0, 0);
    }
}

/*
 * Na impressao de cada face:
 *   direita  = direcao para a coluna da direita
 *   cima     = direcao para a linha de cima
 */
static Vetor3 direitaDaFace(int face)
{
    switch (face)
    {
        case U: return vetor(1, 0, 0);
        case D: return vetor(1, 0, 0);
        case F: return vetor(1, 0, 0);
        case B: return vetor(-1, 0, 0);
        case L: return vetor(0, 0, 1);
        case R: return vetor(0, 0, -1);
        default: return vetor(0, 0, 0);
    }
}

static Vetor3 cimaDaFace(int face)
{
    switch (face)
    {
        case U: return vetor(0, 0, -1);
        case D: return vetor(0, 0, 1);
        case F: return vetor(0, 1, 0);
        case B: return vetor(0, 1, 0);
        case L: return vetor(0, 1, 0);
        case R: return vetor(0, 1, 0);
        default: return vetor(0, 0, 0);
    }
}

/* Converte face + posicao 0..3 para a representacao 3D. */
PosicaoSticker obterPosicaoSticker(int face, int pos)
{
    Vetor3 normal = normalDaFace(face);
    Vetor3 direita = direitaDaFace(face);
    Vetor3 cima = cimaDaFace(face);

    int sinalDireita = (pos == 1 || pos == 3) ? 1 : -1;
    int sinalCima = (pos == 0 || pos == 1) ? 1 : -1;

    PosicaoSticker s;
    s.posicao = somaVetores(
        normal,
        somaVetores(
            multiplicaVetor(direita, sinalDireita),
            multiplicaVetor(cima, sinalCima)
        )
    );
    s.normal = normal;

    return s;
}

/* Encontra face + posicao correspondentes a uma coordenada 3D. */
void encontrarSticker(Vetor3 posicao, Vetor3 normal, int *face, int *pos)
{
    *face = -1;
    *pos = -1;

    for (int f = 0; f < NUM_FACES; f++)
    {
        for (int p = 0; p < STICKERS_POR_FACE; p++)
        {
            PosicaoSticker atual = obterPosicaoSticker(f, p);

            if (vetoresIguais(atual.posicao, posicao) &&
                vetoresIguais(atual.normal, normal))
            {
                *face = f;
                *pos = p;
                return;
            }
        }
    }
}

/* Rotacao de 90 graus em torno de um eixo unitario principal. */
Vetor3 rotacionar90(Vetor3 v, Vetor3 eixo, int sinal)
{
    /* Eixo +X */
    if (vetoresIguais(eixo, vetor(1, 0, 0)))
    {
        if (sinal > 0)
            return vetor(v.x, -v.z, v.y);
        else
            return vetor(v.x, v.z, -v.y);
    }

    /* Eixo -X */
    if (vetoresIguais(eixo, vetor(-1, 0, 0)))
        return rotacionar90(v, vetor(1, 0, 0), -sinal);

    /* Eixo +Y */
    if (vetoresIguais(eixo, vetor(0, 1, 0)))
    {
        if (sinal > 0)
            return vetor(v.z, v.y, -v.x);
        else
            return vetor(-v.z, v.y, v.x);
    }

    /* Eixo -Y */
    if (vetoresIguais(eixo, vetor(0, -1, 0)))
        return rotacionar90(v, vetor(0, 1, 0), -sinal);

    /* Eixo +Z */
    if (vetoresIguais(eixo, vetor(0, 0, 1)))
    {
        if (sinal > 0)
            return vetor(-v.y, v.x, v.z);
        else
            return vetor(v.y, -v.x, v.z);
    }

    /* Eixo -Z */
    if (vetoresIguais(eixo, vetor(0, 0, -1)))
        return rotacionar90(v, vetor(0, 0, 1), -sinal);

    return v;
}

/*
 * Descobre o eixo da camada indicada na face escolhida.
 *
 * C = os 2 blocos de cima
 * B = os 2 blocos de baixo
 * E = os 2 blocos da esquerda
 * D = os 2 blocos da direita
 *
 * O eixo e exatamente a direcao da faixa.
 */
Vetor3 eixoDaFaixa(int face, char faixa)
{
    Vetor3 cima = cimaDaFace(face);
    Vetor3 direita = direitaDaFace(face);

    switch (faixa)
    {
        case 'C': return cima;
        case 'B': return multiplicaVetor(cima, -1);
        case 'E': return multiplicaVetor(direita, -1);
        case 'D': return direita;
        default: return vetor(0, 0, 0);
    }
}

/*
 * Calcula o sinal da rotacao para que H/A seja interpretado
 * olhando diretamente para a face escolhida.
 *
 * Horizontal (C/B):
 *   horario faz a normal da face escolhida ir para a direita.
 *
 * Vertical (E/D):
 *   horario faz a normal da face escolhida ir para baixo.
 *
 * Essa convencao garante, por exemplo, exatamente o movimento pedido:
 *   R + C + H:
 *      R cima -> B cima -> L cima -> F cima -> R cima
 *   e na face U:
 *      sup-esq -> inf-esq -> inf-dir -> sup-dir -> sup-esq
 */
int sinalHorarioDaFaixa(int face, char faixa)
{
    Vetor3 eixo = eixoDaFaixa(face, faixa);
    Vetor3 normal = normalDaFace(face);
    Vetor3 alvo;

    if (faixa == 'C' || faixa == 'B')
        alvo = direitaDaFace(face);
    else
        alvo = multiplicaVetor(cimaDaFace(face), -1);

    Vetor3 resultadoHorario = produtoVetorial(eixo, normal);

    return vetoresIguais(resultadoHorario, alvo) ? 1 : -1;
}

/*
 * Movimenta UMA camada.
 *
 * face     = lado que o usuario escolheu para enxergar a faixa
 * faixa    = C, B, E ou D
 * sentido  = 1 para horario, 0 para anti-horario
 *
 * Apenas os 4 cubinhos da camada escolhida sao alterados.
 */
void moverCamada(int face, char faixa, int sentidoHorario)
{
    if (face < 0 || face >= NUM_FACES)
        return;

    Vetor3 eixo = eixoDaFaixa(face, faixa);

    if (vetoresIguais(eixo, vetor(0, 0, 0)))
        return;

    int sinal = sinalHorarioDaFaixa(face, faixa);
    if (!sentidoHorario)
        sinal = -sinal;

    int novoCubo[NUM_FACES][STICKERS_POR_FACE];

    /* Copia tudo para preservar os stickers que nao pertencem a camada. */
    for (int f = 0; f < NUM_FACES; f++)
        for (int p = 0; p < STICKERS_POR_FACE; p++)
            novoCubo[f][p] = cube[f][p];

    for (int f = 0; f < NUM_FACES; f++)
    {
        for (int p = 0; p < STICKERS_POR_FACE; p++)
        {
            PosicaoSticker origem = obterPosicaoSticker(f, p);

            /* A camada e formada pelos cubinhos cujo centro esta
               no plano definido pelo eixo escolhido. */
            int pertence =
                origem.posicao.x * eixo.x +
                origem.posicao.y * eixo.y +
                origem.posicao.z * eixo.z == 1;

            if (!pertence)
                continue;

            PosicaoSticker destino;
            destino.posicao = rotacionar90(origem.posicao, eixo, sinal);
            destino.normal = rotacionar90(origem.normal, eixo, sinal);

            int novaFace, novaPos;
            encontrarSticker(destino.posicao, destino.normal,
                             &novaFace, &novaPos);

            if (novaFace != -1 && novaPos != -1)
                novoCubo[novaFace][novaPos] = cube[f][p];
        }
    }

    for (int f = 0; f < NUM_FACES; f++)
        for (int p = 0; p < STICKERS_POR_FACE; p++)
            cube[f][p] = novoCubo[f][p];
}

/*
 * Entrada do movimento:
 *   1. lado escolhido;
 *   2. faixa escolhida;
 *   3. sentido.
 */
void escolherMovimento(int *faceEscolhida, char *faixaEscolhida, int *sentidoHorario)
{
    char letraFace;
    char faixa;
    char direcao;

    *faceEscolhida = -1;
    *faixaEscolhida = '\0';
    *sentidoHorario = -1;

    printf("Digite o lado (U, D, L, R, F, B): ");
    scanf(" %c", &letraFace);

    switch (letraFace)
    {
        case 'U': case 'u': *faceEscolhida = U; break;
        case 'D': case 'd': *faceEscolhida = D; break;
        case 'L': case 'l': *faceEscolhida = L; break;
        case 'R': case 'r': *faceEscolhida = R; break;
        case 'F': case 'f': *faceEscolhida = F; break;
        case 'B': case 'b': *faceEscolhida = B; break;
        default:
            printf("Lado invalido!\n");
            return;
    }

    printf("Digite a faixa (C=cima, B=baixo, E=esquerda, D=direita): ");
    scanf(" %c", &faixa);

    switch (faixa)
    {
        case 'C': case 'c': *faixaEscolhida = 'C'; break;
        case 'B': case 'b': *faixaEscolhida = 'B'; break;
        case 'E': case 'e': *faixaEscolhida = 'E'; break;
        case 'D': case 'd': *faixaEscolhida = 'D'; break;
        default:
            printf("Faixa invalida!\n");
            *faceEscolhida = -1;
            return;
    }

    printf("Digite H para horario ou A para anti-horario: ");
    scanf(" %c", &direcao);

    if (direcao == 'H' || direcao == 'h')
        *sentidoHorario = 1;
    else if (direcao == 'A' || direcao == 'a')
        *sentidoHorario = 0;
    else
    {
        printf("Direcao invalida!\n");
        *faceEscolhida = -1;
        *faixaEscolhida = '\0';
        return;
    }

    printf("Movendo %c + faixa %c + %s...\n",
           letraFace,
           *faixaEscolhida,
           *sentidoHorario ? "horario" : "anti-horario");
}




/* ============================================================
 * GERENCIAMENTO DOS ESTADOS USADOS NA COMPARACAO
 * ============================================================ */

void salvarEstadoInicial(void)
{
    memcpy(estadoInicial, cube, sizeof(cube));
    memcpy(estadoComparacao, cube, sizeof(cube));
    possuiEstadoInicial = 1;
    possuiEstadoComparacao = 1;
}

void descartarEstadoInicial(void)
{
    possuiEstadoInicial = 0;
}

void salvarEstadoComparacao(void)
{
    memcpy(estadoComparacao, cube, sizeof(cube));
    possuiEstadoComparacao = 1;

    /*
     * Assim que o usuario movimenta o cubo, o estado inserido originalmente
     * deixa de ser a referencia da comparacao. O estado atual passa a ser a
     * nova referencia para os tres metodos.
     */
    possuiEstadoInicial = 0;
}

void restaurarEstadoComparacao(void)
{
    if (!possuiEstadoComparacao)
        return;

    memcpy(cube, estadoComparacao, sizeof(cube));
}

int existeEstadoComparacao(void)
{
    return possuiEstadoComparacao;
}

int estadoResolvido(void)
{
    for (int f = 0; f < NUM_FACES; f++)
    {
        int cor = cube[f][0];
        for (int p = 1; p < STICKERS_POR_FACE; p++)
            if (cube[f][p] != cor)
                return 0;
    }
    return 1;
}

const char *nomesFaces[NUM_FACES] = {
    "U (Cima)", "D (Baixo)", "F (Frente)",
    "B (Tras)", "L (Esquerda)", "R (Direita)"
};

const char *corParaValor(int valor)
{
    switch (valor)
    {
        case 1: return BG_WHITE;
        case 2: return BG_YELLOW;
        case 3: return BG_GREEN;
        case 4: return BG_BLUE;
        case 5: return BG_ORANGE;
        case 6: return BG_RED;
        default: return BG_UNKNOWN;
    }
}

int lerValorValido(const char *rotulo)
{
    int valor;
    int ok;

    do
    {
        printf("  %s: ", rotulo);
        ok = scanf("%d", &valor);

        if (ok != 1)
        {
            while (getchar() != '\n');
            printf("  Entrada invalida. Digite um numero de 1 a 6.\n");
            valor = -1;
            continue;
        }

        if (valor < 1 || valor > 6)
            printf("  Valor fora do intervalo. Digite um numero de 1 a 6.\n");

    } while (valor < 1 || valor > 6);

    return valor;
}

void lerCubo(void)
{
    const char *rotulosSticker[STICKERS_POR_FACE] = {
        "sup-esq", "sup-dir", "inf-esq", "inf-dir"
    };

    printf("=== Entrada do Cubo 2x2 ===\n");
    printf("Digite valores de 1 a 6 (1=Branco 2=Amarelo 3=Verde 4=Azul 5=Laranja 6=Vermelho)\n\n");

    for (int face = 0; face < NUM_FACES; face++)
    {
        printf("Face %s:\n", nomesFaces[face]);

        for (int sticker = 0; sticker < STICKERS_POR_FACE; sticker++)
            cube[face][sticker] = lerValorValido(rotulosSticker[sticker]);

        printf("\n");
    }
}

int validarCubo(void)
{
    int contagem[7] = {0};

    for (int face = 0; face < NUM_FACES; face++)
        for (int sticker = 0; sticker < STICKERS_POR_FACE; sticker++)
            if (cube[face][sticker] >= 1 && cube[face][sticker] <= 6)
                contagem[cube[face][sticker]]++;

    int valido = 1;

    for (int cor = 1; cor <= 6; cor++)
    {
        if (contagem[cor] != 4)
        {
            printf("Aviso: cor %d aparece %d vezes (esperado: 4).\n",
                   cor, contagem[cor]);
            valido = 0;
        }
    }

    return valido;
}

void printSquare(int valor)
{
    printf("%s  %s", corParaValor(valor), RESET);
}

void printGap(int count)
{
    for (int i = 0; i < count; i++)
        printf("  ");
}

void imprimirCubo(void)
{
    printf("\n=== Cubo Atual (planificado) ===\n\n");

    printGap(3);
    printSquare(cube[U][0]); printSquare(cube[U][1]);
    printf("\n");

    printGap(3);
    printSquare(cube[U][2]); printSquare(cube[U][3]);
    printf("\n");

    printSquare(cube[L][0]); printSquare(cube[L][1]);
    printf("  ");
    printSquare(cube[F][0]); printSquare(cube[F][1]);
    printf("  ");
    printSquare(cube[R][0]); printSquare(cube[R][1]);
    printf("  ");
    printSquare(cube[B][0]); printSquare(cube[B][1]);
    printf("\n");

    printSquare(cube[L][2]); printSquare(cube[L][3]);
    printf("  ");
    printSquare(cube[F][2]); printSquare(cube[F][3]);
    printf("  ");
    printSquare(cube[R][2]); printSquare(cube[R][3]);
    printf("  ");
    printSquare(cube[B][2]); printSquare(cube[B][3]);
    printf("\n");

    printGap(3);
    printSquare(cube[D][0]); printSquare(cube[D][1]);
    printf("\n");

    printGap(3);
    printSquare(cube[D][2]); printSquare(cube[D][3]);
    printf("\n\n");
}

void imprimirMapaFaces(void)
{
    printf("\n=== Mapa de Faces (orientacao fixa) ===\n");
    printf("U = face de cima, D = face de baixo, F = face da frente,\n");
    printf("B = face de tras, L = face da esquerda, R = face da direita.\n\n");

    printf("            U U\n");
    printf("            U U\n");
    printf("L L  F F  R R  B B\n");
    printf("L L  F F  R R  B B\n");
    printf("            D D\n");
    printf("            D D\n\n");

    printf("Legenda:\n");
    for (int face = 0; face < NUM_FACES; face++)
        printf("  %s\n", nomesFaces[face]);

    printf("\n");
}

