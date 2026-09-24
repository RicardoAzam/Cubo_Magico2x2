#ifndef CUBO_H
#define CUBO_H

#define NUM_FACES 6
#define STICKERS_POR_FACE 4

#define U 0
#define D 1
#define F 2
#define B 3
#define L 4
#define R 5

#define BG_WHITE   "\033[47m"
#define BG_YELLOW  "\033[43m"
#define BG_GREEN   "\033[42m"
#define BG_BLUE    "\033[44m"
#define BG_ORANGE  "\033[48;5;208m"
#define BG_RED     "\033[41m"
#define BG_UNKNOWN "\033[100m"
#define RESET      "\033[0m"

typedef struct
{
    int face;
    int pos;
} Sticker;

typedef struct
{
    int x;
    int y;
    int z;
} Vetor3;

typedef struct
{
    Vetor3 posicao;
    Vetor3 normal;
} PosicaoSticker;

typedef struct
{
    int face;
    char faixa;
    int sentido;
} Movimento;

extern int cube[NUM_FACES][STICKERS_POR_FACE];
extern const Vetor3 coordenadasCantos[8];

/* Representacao e entrada */
void lerCubo(void);
int validarCubo(void);

/* Impressao */
void printSquare(int valor);
void printGap(int count);
void imprimirCubo(void);
void imprimirMapaFaces(void);
const char *corParaValor(int valor);

/* Movimentacao */
void escolherMovimento(int *faceEscolhida, char *faixaEscolhida, int *sentidoHorario);
void moverCamada(int face, char faixa, int sentidoHorario);
int estadoResolvido(void);

/* Estados auxiliares usados para comparar os metodos de resolucao. */
void salvarEstadoInicial(void);
void descartarEstadoInicial(void);
void salvarEstadoComparacao(void);
void restaurarEstadoComparacao(void);
int existeEstadoComparacao(void);

/* Funcoes geometricas usadas pelo solucionador. */
Vetor3 vetor(int x, int y, int z);
int vetoresIguais(Vetor3 a, Vetor3 b);
Vetor3 multiplicaVetor(Vetor3 a, int k);
PosicaoSticker obterPosicaoSticker(int face, int pos);
void encontrarSticker(Vetor3 posicao, Vetor3 normal, int *face, int *pos);
Vetor3 rotacionar90(Vetor3 v, Vetor3 eixo, int sinal);
Vetor3 eixoDaFaixa(int face, char faixa);
int sinalHorarioDaFaixa(int face, char faixa);
int indiceCantoPorCoordenada(Vetor3 v);
int indiceStickerLinear(int face, int pos);

#endif
