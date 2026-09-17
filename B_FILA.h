#ifndef B_FILA_H
#define B_FILA_H

#include <stdio.h>
#include <stdlib.h>

/*CONFIGURACOES DO CUBO*/

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

/*
    Convencao dos stickers:

    0 = superior esquerdo
    1 = superior direito
    2 = inferior esquerdo
    3 = inferior direito
*/

int cube[NUM_FACES][STICKERS_POR_FACE];

const char *nomesFaces[NUM_FACES] =
{
    "U (Cima)",
    "D (Baixo)",
    "F (Frente)",
    "B (Tras)",
    "L (Esquerda)",
    "R (Direita)"
};

/*ESTRUTURAS*/

typedef struct
{
    int face;
    int pos;
} Sticker;

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

typedef struct
{
    Sticker propria[4];
    Sticker banda1[4];
    Sticker banda2[4];
} AneisFace;

/*TABELA DE MOVIMENTOS DAS FACES*/

AneisFace aneisPorFace[NUM_FACES] =
{
    [U] =
    {
        .propria =
        {
            {U, 1},
            {U, 3},
            {U, 2},
            {U, 0}
        },

        .banda1 =
        {
            {F, 0},
            {L, 0},
            {B, 0},
            {R, 0}
        },

        .banda2 =
        {
            {F, 1},
            {L, 1},
            {B, 1},
            {R, 1}
        }
    },

    [D] =
    {
        .propria =
        {
            {D, 0},
            {D, 2},
            {D, 3},
            {D, 1}
        },

        .banda1 =
        {
            {L, 3},
            {B, 3},
            {R, 3},
            {F, 3}
        },

        .banda2 =
        {
            {L, 2},
            {B, 2},
            {R, 2},
            {F, 2}
        }
    },

    [F] =
    {
        .propria =
        {
            {F, 1},
            {F, 3},
            {F, 2},
            {F, 0}
        },

        .banda1 =
        {
            {U, 2},
            {R, 0},
            {D, 1},
            {L, 3}
        },

        .banda2 =
        {
            {U, 3},
            {R, 2},
            {D, 0},
            {L, 1}
        }
    },

    [B] =
    {
        .propria =
        {
            {B, 0},
            {B, 2},
            {B, 3},
            {B, 1}
        },

        .banda1 =
        {
            {U, 0},
            {R, 1},
            {D, 3},
            {L, 2}
        },

        .banda2 =
        {
            {U, 1},
            {R, 3},
            {D, 2},
            {L, 0}
        }
    },

    [L] =
    {
        .propria =
        {
            {L, 0},
            {L, 2},
            {L, 3},
            {L, 1}
        },

        .banda1 =
        {
            {U, 0},
            {B, 3},
            {D, 0},
            {F, 0}
        },

        .banda2 =
        {
            {U, 2},
            {B, 1},
            {D, 2},
            {F, 2}
        }
    },

    [R] =
    {
        .propria =
        {
            {R, 0},
            {R, 1},
            {R, 3},
            {R, 2}
        },

        .banda1 =
        {
            {U, 3},
            {B, 0},
            {D, 3},
            {F, 2}
        },

        .banda2 =
        {
            {U, 1},
            {B, 2},
            {D, 1},
            {F, 1}
        }
    }
};

/*FUNCOES DA FILA*/

Fila *CriaFila(void)
{
    Fila *f = (Fila *) malloc(sizeof(Fila));

    if (f == NULL)
    {
        printf("Erro ao alocar memoria para a fila.\n");
        exit(1);
    }

    f->ini = NULL;
    f->fim = NULL;

    return f;
}

Nos *ins_fim(Nos *fim, int valor)
{
    Nos *novo = (Nos *) malloc(sizeof(Nos));

    if (novo == NULL)
    {
        printf("Erro ao alocar memoria para o no.\n");
        exit(1);
    }

    novo->info = valor;
    novo->prox = NULL;

    if (fim != NULL)
        fim->prox = novo;

    return novo;
}

void InsereFila(Fila *f, int valor)
{
    f->fim = ins_fim(f->fim, valor);

    if (f->ini == NULL)
        f->ini = f->fim;
}

int removeInicio(Fila *f)
{
    Nos *removido;
    int valor;

    if (f == NULL || f->ini == NULL)
        return -1;

    removido = f->ini;
    valor = removido->info;

    f->ini = removido->prox;

    if (f->ini == NULL)
        f->fim = NULL;

    free(removido);

    return valor;
}

void rotacionaFila(Fila *f)
{
    int valor;

    if (f == NULL || f->ini == NULL)
        return;

    valor = removeInicio(f);
    InsereFila(f, valor);
}

void destroiFila(Fila *f)
{
    Nos *atual;
    Nos *proximo;

    if (f == NULL)
        return;

    atual = f->ini;

    while (atual != NULL)
    {
        proximo = atual->prox;
        free(atual);
        atual = proximo;
    }

    free(f);
}

void imprimeFila(Fila *f)
{
    Nos *atual;
    int contador = 0;

    if (f == NULL || f->ini == NULL)
    {
        printf("Fila vazia.\n");
        return;
    }

    printf("\nFila:\n");

    atual = f->ini;

    while (atual != NULL)
    {
        printf("%d ", atual->info);

        contador++;

        if (contador % 4 == 0)
            printf("\n");

        atual = atual->prox;
    }

    printf("\n");
}

/*FUNCOES DE LEITURA E VALIDACAO DO CUBO*/

int lerValorValido(const char *rotulo)
{
    int valor;
    int resultado;

    do
    {
        printf("  %s: ", rotulo);

        resultado = scanf("%d", &valor);

        if (resultado != 1)
        {
            while (getchar() != '\n');

            printf("  Entrada invalida. Digite um numero de 1 a 6.\n");
            valor = -1;
        }
        else if (valor < 1 || valor > 6)
        {
            printf("  Valor fora do intervalo. Digite um numero de 1 a 6.\n");
        }

    } while (valor < 1 || valor > 6);

    return valor;
}

void lerCubo(void)
{
    const char *rotulosSticker[STICKERS_POR_FACE] =
    {
        "sup-esq",
        "sup-dir",
        "inf-esq",
        "inf-dir"
    };

    int face;
    int sticker;

    printf("\n=== Entrada do Cubo 2x2 ===\n");
    printf("Digite valores de 1 a 6:\n");
    printf("1 = Branco | 2 = Amarelo | 3 = Verde\n");
    printf("4 = Azul | 5 = Laranja | 6 = Vermelho\n\n");

    for (face = 0; face < NUM_FACES; face++)
    {
        printf("Face %s:\n", nomesFaces[face]);

        for (sticker = 0; sticker < STICKERS_POR_FACE; sticker++)
        {
            cube[face][sticker] =
                lerValorValido(rotulosSticker[sticker]);
        }

        printf("\n");
    }
}

int validarCubo(void)
{
    int contagem[7] = {0};
    int face;
    int sticker;
    int cor;
    int valido = 1;

    for (face = 0; face < NUM_FACES; face++)
    {
        for (sticker = 0; sticker < STICKERS_POR_FACE; sticker++)
        {
            contagem[cube[face][sticker]]++;
        }
    }

    for (cor = 1; cor <= 6; cor++)
    {
        if (contagem[cor] != 4)
        {
            printf(
                "Aviso: cor %d aparece %d vezes. Esperado: 4.\n",
                cor,
                contagem[cor]
            );

            valido = 0;
        }
    }

    return valido;
}

/*FUNCOES DE IMPRESSAO*/

const char *corParaValor(int valor)
{
    switch (valor)
    {
        case 1:
            return BG_WHITE;

        case 2:
            return BG_YELLOW;

        case 3:
            return BG_GREEN;

        case 4:
            return BG_BLUE;

        case 5:
            return BG_ORANGE;

        case 6:
            return BG_RED;

        default:
            return BG_UNKNOWN;
    }
}

void printSquare(int valor)
{
    printf("%s  %s", corParaValor(valor), RESET);
}

void printGap(int quantidade)
{
    int i;

    for (i = 0; i < quantidade; i++)
        printf("  ");
}

void imprimirCubo(void)
{
    printf("\n=== Cubo Atual (planificado) ===\n\n");

    /* Face U */
    printGap(2);
    printSquare(cube[U][0]);
    printSquare(cube[U][1]);
    printf("\n");

    printGap(2);
    printSquare(cube[U][2]);
    printSquare(cube[U][3]);
    printf("\n");

    /* Faces L, F, R e B - parte superior */
    printSquare(cube[L][0]);
    printSquare(cube[L][1]);

    printSquare(cube[F][0]);
    printSquare(cube[F][1]);

    printSquare(cube[R][0]);
    printSquare(cube[R][1]);

    printSquare(cube[B][0]);
    printSquare(cube[B][1]);

    printf("\n");

    /* Faces L, F, R e B - parte inferior */
    printSquare(cube[L][2]);
    printSquare(cube[L][3]);

    printSquare(cube[F][2]);
    printSquare(cube[F][3]);

    printSquare(cube[R][2]);
    printSquare(cube[R][3]);

    printSquare(cube[B][2]);
    printSquare(cube[B][3]);

    printf("\n");

    /* Face D */
    printGap(2);
    printSquare(cube[D][0]);
    printSquare(cube[D][1]);
    printf("\n");

    printGap(2);
    printSquare(cube[D][2]);
    printSquare(cube[D][3]);
    printf("\n\n");
}

void imprimirMapaFaces(void)
{
    int face;

    printf("\n=== Mapa de Faces (orientacao fixa) ===\n");
    printf(
        "Segure o cubo com a face BRANCA virada para voce "
        "(F) e a face AMARELA em cima (U).\n\n"
    );

    printf("        U U\n");
    printf("        U U\n");
    printf("L L | F F | R R | B B\n");
    printf("L L | F F | R R | B B\n");
    printf("        D D\n");
    printf("        D D\n\n");

    printf("Legenda:\n");

    for (face = 0; face < NUM_FACES; face++)
        printf("  %s\n", nomesFaces[face]);

    printf("\n");
}

/*FUNCOES PARA MONTAR E ATUALIZAR ANEIS*/

Fila *montaAnel(Fila *f, Sticker anel[8])
{
    int i;

    for (i = 0; i < 8; i++)
    {
        InsereFila(
            f,
            cube[anel[i].face][anel[i].pos]
        );
    }

    return f;
}

void atualizaCubo(Fila *f, Sticker anel[8])
{
    Nos *atual;
    int i = 0;

    if (f == NULL)
        return;

    atual = f->ini;

    while (atual != NULL && i < 8)
    {
        cube[anel[i].face][anel[i].pos] = atual->info;

        atual = atual->prox;
        i++;
    }
}

void montaAnel4(Fila *f, Sticker anel[4])
{
    int i;

    for (i = 0; i < 4; i++)
    {
        InsereFila(
            f,
            cube[anel[i].face][anel[i].pos]
        );
    }
}

void atualizaCubo4(Fila *f, Sticker anel[4])
{
    Nos *atual;
    int i = 0;

    if (f == NULL)
        return;

    atual = f->ini;

    while (atual != NULL && i < 4)
    {
        cube[anel[i].face][anel[i].pos] = atual->info;

        atual = atual->prox;
        i++;
    }
}

/* FUNCOES DE ROTACAO */

void giraAnel(Sticker anel[4], int vezes)
{
    Fila *f;
    int i;

    f = CriaFila();

    montaAnel4(f, anel);

    for (i = 0; i < vezes; i++)
        rotacionaFila(f);

    atualizaCubo4(f, anel);

    destroiFila(f);
}

void girarFace(int face, int sentidoHorario)
{
    int vezes;

    if (face < 0 || face >= NUM_FACES)
    {
        printf("Face invalida!\n");
        return;
    }

    vezes = sentidoHorario ? 1 : 3;

    giraAnel(aneisPorFace[face].propria, vezes);
    giraAnel(aneisPorFace[face].banda1, vezes);
    giraAnel(aneisPorFace[face].banda2, vezes);
}

void girarFaceSetor(
    int face,
    int setor,
    int sentidoHorario,
    int sentidoVertical
)
{
    int vezes;

    if (face < 0 || face >= NUM_FACES)
    {
        printf("Face invalida!\n");
        return;
    }

    if (setor < 1 || setor > 4)
    {
        printf("Setor invalido!\n");
        return;
    }

    /*
        Setores 1 e 2 representam as partes superior
        e inferior. Neles usamos horario ou anti-horario.
    */
    if (setor == 1 || setor == 2)
    {
        vezes = sentidoHorario ? 1 : 3;

        /*
            A face propria e rotacionada para que a alteracao
            possa ser visualizada no cubo.
        */
        giraAnel(aneisPorFace[face].propria, vezes);

        if (setor == 1)
            giraAnel(aneisPorFace[face].banda1, vezes);
        else
            giraAnel(aneisPorFace[face].banda2, vezes);

        return;
    }

    /*
        Setores 3 e 4 representam as partes laterais.
        Neles usamos para cima ou para baixo.
    */
    vezes = sentidoVertical ? 1 : 3;

    if (setor == 3)
        giraAnel(aneisPorFace[face].banda1, vezes);
    else
        giraAnel(aneisPorFace[face].banda2, vezes);
}

/*LEITURA DO MOVIMENTO*/

void escolherMovimento(
    int *faceEscolhida,
    int *setor,
    int *sentidoHorario,
    int *sentidoVertical
)
{
    char letra;
    char sentido;

    *faceEscolhida = -1;
    *setor = 0;
    *sentidoHorario = 0;
    *sentidoVertical = 0;

    printf("\nDigite a face (U, D, F, B, L, R): ");
    scanf(" %c", &letra);

    switch (letra)
    {
        case 'U':
        case 'u':
            *faceEscolhida = U;
            break;

        case 'D':
        case 'd':
            *faceEscolhida = D;
            break;

        case 'F':
        case 'f':
            *faceEscolhida = F;
            break;

        case 'B':
        case 'b':
            *faceEscolhida = B;
            break;

        case 'L':
        case 'l':
            *faceEscolhida = L;
            break;

        case 'R':
        case 'r':
            *faceEscolhida = R;
            break;

        default:
            printf("Face invalida!\n");
            return;
    }

    printf("\nQual setor do cubo deseja movimentar?\n");
    printf("1- Os dois lados de cima\n");
    printf("2- Os dois lados de baixo\n");
    printf("3- Os dois lados direitos\n");
    printf("4- Os dois lados esquerdos\n");
    printf("Escolha: ");

    if (scanf("%d", setor) != 1)
    {
        while (getchar() != '\n');

        printf("Setor invalido!\n");
        *faceEscolhida = -1;
        return;
    }

    if (*setor < 1 || *setor > 4)
    {
        printf("Setor invalido!\n");
        *faceEscolhida = -1;
        return;
    }

    /*
        Para os setores de cima e de baixo,
        perguntar horario ou anti-horario.
    */
    if (*setor == 1 || *setor == 2)
    {
        printf(
            "Digite A para horario ou S para anti-horario: "
        );

        scanf(" %c", &sentido);

        if (sentido == 'A' || sentido == 'a')
        {
            *sentidoHorario = 1;
        }
        else if (sentido == 'S' || sentido == 's')
        {
            *sentidoHorario = 0;
        }
        else
        {
            printf("Sentido invalido!\n");
            *faceEscolhida = -1;
        }

        return;
    }

    /*
        Para os setores laterais,
        perguntar para cima ou para baixo.
    */
    printf(
        "Digite C para movimentar para cima "
        "ou B para movimentar para baixo: "
    );

    scanf(" %c", &sentido);

    if (sentido == 'C' || sentido == 'c')
    {
        *sentidoVertical = 1;
    }
    else if (sentido == 'B' || sentido == 'b')
    {
        *sentidoVertical = 0;
    }
    else
    {
        printf("Direcao invalida!\n");
        *faceEscolhida = -1;
    }
}

/*FUNCAO AUXILIAR DE RESOLUCAO*/

void resolverCubo(Fila *f, Sticker anel[8])
{
    int opcao;

    if (f == NULL)
        f = CriaFila();

    montaAnel(f, anel);

    opcao = 1;

    while (opcao == 1)
    {
        printf("\nDigite 1 para rotacionar o anel.\n");
        printf("Digite 0 para sair.\n");
        printf("Escolha: ");

        scanf("%d", &opcao);

        if (opcao == 1)
        {
            rotacionaFila(f);
            atualizaCubo(f, anel);

            printf("\nCubo apos a rotacao:\n");
            imprimirCubo();
        }
    }
}


#endif // B_FILA_H