#ifndef B_FILA_H
#define B_FILA_H

#include <stdio.h>
#include <stdlib.h>

/*
 * Cubo Magico 2x2 - Entrada numerica + Impressao (planificado, ANSI)
 * --------------------------------------------------------------
 * Convencao:
 *   Faces (indice):      0=U  1=D  2=F  3=B  4=L  5=R
 *   Stickers por face:   0=top-left  1=top-right  2=bottom-left  3=bottom-right
 *   Cores (valor 1-6):   1=Branco  2=Amarelo  3=Verde  4=Azul  5=Laranja  6=Vermelho
 *
 * O estado do cubo fica em cube[6][4], cada posicao guardando um valor de 1 a 6.
 */
 
#define NUM_FACES 6
#define STICKERS_POR_FACE 4
 
#define U 0
#define D 1
#define F 2
#define B 3
#define L 4
#define R 5
 
// Codigos ANSI de fundo colorido
#define BG_WHITE   "\033[47m"
#define BG_YELLOW  "\033[43m"
#define BG_GREEN   "\033[42m"
#define BG_BLUE    "\033[44m"
#define BG_ORANGE  "\033[48;5;208m"
#define BG_RED     "\033[41m"
#define BG_UNKNOWN "\033[100m" // cinza, usado se algo der errado
#define RESET      "\033[0m"

typedef struct { int face; int pos; } Sticker;

// Precisa vir antes de qualquer funcao que use cube[][] (ex: montaAnel, atualizaCubo)
int cube[NUM_FACES][STICKERS_POR_FACE];

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

Fila* CriaFila()
{
    Fila* f = (Fila*) malloc(sizeof(Fila));
    f->ini = f->fim = NULL;
    return f;
}

Nos* ins_fim(Nos *fim, int A)
{
    Nos* p = (Nos*) malloc(sizeof(Nos));

    p->info = A;
    p->prox = NULL;

    if (fim != NULL)
        fim->prox = p;

    return p;
}

void InsereFila(Fila* f, int v)
{
    f->fim = ins_fim(f->fim, v);

    if (f->ini == NULL)
        f->ini = f->fim;
}

int removeInicio(Fila* f) {
    if (f->ini == NULL) return -1; // fila vazia

    Nos* removido = f->ini;
    int valor = removido->info;

    f->ini = f->ini->prox;
    if (f->ini == NULL) f->fim = NULL; // fila ficou vazia

    free(removido);
    return valor;
}

Fila* montaAnel(Fila *f, Sticker anel[8]) {
    for (int i = 0; i < 8; i++)
        InsereFila(f, cube[anel[i].face][anel[i].pos]);
    return f;
}

void rotacionaFila(Fila* f) {
    int valor = removeInicio(f);
    InsereFila(f, valor);
}

void atualizaCubo(Fila* f, Sticker anel[8]) {
    Nos* atual = f->ini;
    int i = 0;
    while (atual != NULL && i < 8) {
        cube[anel[i].face][anel[i].pos] = atual->info;
        atual = atual->prox;
        i++;
    }
}

// ============================================================
// Tabela de aneis por face: cada movimento afeta 3 grupos de 4
// stickers (a propria face + dois "sub-aneis" das faces vizinhas).
// Cada grupo gira de forma independente com o mesmo mecanismo
// (1x rotacionaFila = horario, 3x = anti-horario).
// ============================================================

typedef struct {
    Sticker propria[4];
    Sticker banda1[4];
    Sticker banda2[4];
} AneisFace;

AneisFace aneisPorFace[NUM_FACES] = {
    [U] = {
        .propria = { {U,1}, {U,3}, {U,2}, {U,0} },
        .banda1  = { {F,0}, {L,0}, {B,0}, {R,0} },
        .banda2  = { {F,1}, {L,1}, {B,1}, {R,1} }
    },
    [D] = {
        .propria = { {D,0}, {D,2}, {D,3}, {D,1} },
        .banda1  = { {L,3}, {B,3}, {R,3}, {F,3} },
        .banda2  = { {L,2}, {B,2}, {R,2}, {F,2} }
    },
    [F] = {
        .propria = { {F,1}, {F,3}, {F,2}, {F,0} },
        .banda1  = { {U,2}, {R,0}, {D,1}, {L,3} },
        .banda2  = { {U,3}, {R,2}, {D,0}, {L,1} }
    },
    [B] = {
        .propria = { {B,0}, {B,2}, {B,3}, {B,1} },
        .banda1  = { {U,0}, {R,1}, {D,3}, {L,2} },
        .banda2  = { {U,1}, {R,3}, {D,2}, {L,0} }
    },
    [L] = {
        .propria = { {L,0}, {L,2}, {L,3}, {L,1} },
        .banda1  = { {U,0}, {B,3}, {D,0}, {F,0} },
        .banda2  = { {U,2}, {B,1}, {D,2}, {F,2} }
    },
    [R] = {
        .propria = { {R,0}, {R,1}, {R,3}, {R,2} },
        .banda1  = { {U,3}, {B,0}, {D,3}, {F,2} },
        .banda2  = { {U,1}, {B,2}, {D,1}, {F,1} }
    }
};

// Versoes de montaAnel/atualizaCubo para grupos de 4 (em vez de 8)
void montaAnel4(Fila *f, Sticker anel[4]) {
    for (int i = 0; i < 4; i++)
        InsereFila(f, cube[anel[i].face][anel[i].pos]);
}

void atualizaCubo4(Fila *f, Sticker anel[4]) {
    Nos *atual = f->ini;
    int i = 0;
    while (atual != NULL && i < 4) {
        cube[anel[i].face][anel[i].pos] = atual->info;
        atual = atual->prox;
        i++;
    }
}

// Libera todos os nos de uma fila (a fila e so uma ferramenta temporaria)
void destroiFila(Fila *f) {
    Nos *atual = f->ini;
    while (atual != NULL) {
        Nos *prox = atual->prox;
        free(atual);
        atual = prox;
    }
    free(f);
}

// Gira um unico grupo de 4 stickers "vezes" vezes (1=horario, 3=anti-horario)
void giraAnel(Sticker anel[4], int vezes) {
    Fila *f = CriaFila();
    montaAnel4(f, anel);

    for (int i = 0; i < vezes; i++)
        rotacionaFila(f);

    atualizaCubo4(f, anel);
    destroiFila(f);
}

// Gira uma face inteira: os 3 grupos (propria + 2 bandas) juntos
void girarFace(int face, int sentidoHorario) {
    int vezes = sentidoHorario ? 1 : 3;

    giraAnel(aneisPorFace[face].propria, vezes);
    giraAnel(aneisPorFace[face].banda1, vezes);
    giraAnel(aneisPorFace[face].banda2, vezes);
}

// Le do usuario qual face girar e em qual sentido.
// faceEscolhida recebe -1 se a entrada for invalida.
void escolherMovimento(int *faceEscolhida, int *sentidoHorario) {
    char letra;
    char extra;

    printf("Digite a face (U, D, F, B, L, R): ");
    scanf(" %c", &letra);

    switch (letra) {
        case 'U': case 'u': *faceEscolhida = U; break;
        case 'D': case 'd': *faceEscolhida = D; break;
        case 'F': case 'f': *faceEscolhida = F; break;
        case 'B': case 'b': *faceEscolhida = B; break;
        case 'L': case 'l': *faceEscolhida = L; break;
        case 'R': case 'r': *faceEscolhida = R; break;
        default:
            printf("Face invalida!\n");
            *faceEscolhida = -1;
            return;
    }

    printf("Digite 'b' para anti-horario, ou 'a' para horario: ");
    extra = getchar(); // pega o que sobrou do buffer (o \n do scanf anterior)
    if (extra != 'a') {
        *sentidoHorario = (extra == 'b') ? 0 : 1;
        while (getchar() != '\n'); // limpa o resto da linha
    } else {
        *sentidoHorario = 1; // sem apostrofo = horario
    }
}

void imprimeFila(Fila* f)
{
    Nos* q;
    int i=0,j=0,k;
    printf("\n");

    for (q = f->ini; q != NULL; q = q->prox)
    {
      if (i == 2)
      {
        printf("\n");
        i = 0;
      }
      if(j==4)
      {
        printf("\n\n");
        j=0;
      }
      printf("%d - ", q->info);  
    i++;
    j++;
    }
    printf("\n");
}

// Protótipo: a definição completa só vem mais abaixo no arquivo,
// mas resolverCubo() precisa chamá-la antes disso.
void imprimirCubo(void);
void imprimirMapaFaces(void);

void resolverCubo(Fila* f, Sticker anel[8])
{
    Nos* atual = f->ini;
    int x; 
    montaAnel(f, anel);
    while(x==1)
    {
        printf("Digite 1 para rotacionar o anel e 0 para sair: ");
        scanf("%d", &x);
        if(x==1)
        {
            rotacionaFila(f);
            atualizaCubo(f, anel);
            printf("\nCubo apos rotacao:\n");
            imprimirCubo();
        }
    }
}


 
const char *nomesFaces[NUM_FACES] = {"U (Cima)", "D (Baixo)", "F (Frente)", "B (Tras)", "L (Esquerda)", "R (Direita)"};
 
// Converte valor 1-6 para o codigo de cor ANSI correspondente
const char *corParaValor(int valor) {
    switch (valor) {
        case 1: return BG_WHITE;
        case 2: return BG_YELLOW;
        case 3: return BG_GREEN;
        case 4: return BG_BLUE;
        case 5: return BG_ORANGE;
        case 6: return BG_RED;
        default: return BG_UNKNOWN;
    }
}
 
// Le um valor inteiro de 1 a 6 do usuario, repetindo ate ser valido
int lerValorValido(const char *rotulo) {
    int valor;
    int ok;
 
    do {
        printf("  %s: ", rotulo);
        ok = scanf("%d", &valor);
 
        if (ok != 1) {
            // limpa entrada invalida (ex: usuario digitou letra)
            while (getchar() != '\n');
            printf("  Entrada invalida. Digite um numero de 1 a 6.\n");
            valor = -1;
            continue;
        }
 
        if (valor < 1 || valor > 6) {
            printf("  Valor fora do intervalo. Digite um numero de 1 a 6.\n");
        }
    } while (valor < 1 || valor > 6);
 
    return valor;
}
 
// Pede ao usuario os 24 valores do cubo, face por face
void lerCubo(void) {
    const char *rotulosSticker[STICKERS_POR_FACE] = {
        "sup-esq", "sup-dir", "inf-esq", "inf-dir"
    };
 
    printf("=== Entrada do Cubo 2x2 ===\n");
    printf("Digite valores de 1 a 6 (1=Branco 2=Amarelo 3=Verde 4=Azul 5=Laranja 6=Vermelho)\n\n");
 
    for (int face = 0; face < NUM_FACES; face++) {
        printf("Face %s:\n", nomesFaces[face]);
        for (int sticker = 0; sticker < STICKERS_POR_FACE; sticker++) {
            cube[face][sticker] = lerValorValido(rotulosSticker[sticker]);
        }
        printf("\n");
    }
}
 
// Valida se o cubo tem exatamente 4 stickers de cada cor (regra fisica basica)
int validarCubo(void) {
    int contagem[7] = {0}; // indices 1 a 6 usados
 
    for (int face = 0; face < NUM_FACES; face++) {
        for (int sticker = 0; sticker < STICKERS_POR_FACE; sticker++) {
            contagem[cube[face][sticker]]++;
        }
    }
 
    int valido = 1;
    for (int cor = 1; cor <= 6; cor++) {
        if (contagem[cor] != 4) {
            printf("Aviso: cor %d aparece %d vezes (esperado: 4).\n", cor, contagem[cor]);
            valido = 0;
        }
    }
 
    return valido;
}
 
// Imprime um "quadradinho" colorido
void printSquare(int valor) {
    printf("%s  %s", corParaValor(valor), RESET);
}
 
void printGap(int count) {
    for (int i = 0; i < count; i++) printf("  ");
}
 
// Imprime o cubo planificado (net) com base no estado atual em cube[][]
void imprimirCubo(void) {
    printf("\n=== Cubo Atual (planificado) ===\n\n");
 
    // Face U
    printGap(2);
    printSquare(cube[U][0]); printSquare(cube[U][1]);
    printf("\n");
    printGap(2);
    printSquare(cube[U][2]); printSquare(cube[U][3]);
    printf("\n");
 
    // Faces L, F, R, B lado a lado (linha de cima de cada uma)
    printSquare(cube[L][0]); printSquare(cube[L][1]);
    printSquare(cube[F][0]); printSquare(cube[F][1]);
    printSquare(cube[R][0]); printSquare(cube[R][1]);
    printSquare(cube[B][0]); printSquare(cube[B][1]);
    printf("\n");
 
    // Faces L, F, R, B lado a lado (linha de baixo de cada uma)
    printSquare(cube[L][2]); printSquare(cube[L][3]);
    printSquare(cube[F][2]); printSquare(cube[F][3]);
    printSquare(cube[R][2]); printSquare(cube[R][3]);
    printSquare(cube[B][2]); printSquare(cube[B][3]);
    printf("\n");
 
    // Face D
    printGap(2);
    printSquare(cube[D][0]); printSquare(cube[D][1]);
    printf("\n");
    printGap(2);
    printSquare(cube[D][2]); printSquare(cube[D][3]);
    printf("\n\n");
}

// Imprime o mesmo layout planificado, mas com as letras das faces
// em vez das cores, para o usuario se orientar antes de girar.
void imprimirMapaFaces(void) {
    printf("\n=== Mapa de Faces (orientacao fixa) ===\n");
    printf("Segure o cubo com a face BRANCA virada para voce (F) e a face AMARELA em cima (U).\n\n");

    printf("        U U\n");
    printf("        U U\n");
    printf("L L | F F | R R | B B\n");
    printf("L L | F F | R R | B B\n");
    printf("        D D\n");
    printf("        D D\n\n");

    printf("Legenda:\n");
    for (int face = 0; face < NUM_FACES; face++) {
        printf("  %s\n", nomesFaces[face]);
    }
    printf("\n");
}
 
#endif // B_FILA_H