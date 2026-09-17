// main.c
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include "B_FILA.h"

int main(void)
{
    int opcao;

    setlocale(LC_ALL, "Portuguese");

    while (1)
    {
        printf("\n====================================\n");
        printf("          CUBO MAGICO 2x2\n");
        printf("====================================\n");
        printf("1 - Visualizar metodo de resolucao\n");
        printf("2 - Tentar resolver o cubo\n");
        printf("3 - Mover manualmente\n");
        printf("4 - Sair\n");
        printf("Escolha uma opcao: ");

        if (scanf("%d", &opcao) != 1)
        {
            while (getchar() != '\n');

            printf("\nOpcao invalida!\n");
            continue;
        }

        switch (opcao)
        {
            case 1:
                printf("\nMetodo de resolucao ainda nao implementado.\n");
                break;

            case 2:
            case 3:
            {
                int faceEscolhida;
                int setor;
                int sentidoHorario;
                int sentidoVertical;

                lerCubo();

                /*
                 * Solicita novamente o cubo enquanto ele
                 * nao possuir quatro stickers de cada cor.
                 */
                while (!validarCubo())
                {
                    printf("\nAtencao: o cubo digitado e invalido.\n");
                    printf("Cada cor deve aparecer exatamente 4 vezes.\n");
                    printf("Digite o cubo novamente.\n\n");

                    lerCubo();
                }

                imprimirCubo();
                imprimirMapaFaces();

                while (1)
                {
                    escolherMovimento(
                        &faceEscolhida,
                        &setor,
                        &sentidoHorario,
                        &sentidoVertical
                    );

                    /*
                     * Quando a entrada for invalida,
                     * permite tentar novamente ou sair.
                     */
                    if (faceEscolhida == -1)
                    {
                        printf("\n1 - Tentar novamente\n");
                        printf("2 - Sair\n");
                        printf("Escolha: ");

                        if (scanf("%d", &opcao) != 1)
                        {
                            while (getchar() != '\n');
                            opcao = 1;
                        }

                        if (opcao == 2)
                            break;

                        continue;
                    }

                    /*
                     * Executa o movimento escolhido.
                     */
                    girarFaceSetor(
                        faceEscolhida,
                        setor,
                        sentidoHorario,
                        sentidoVertical
                    );

                    printf("\nCubo atualizado:\n");
                    imprimirCubo();

                    printf("\nMapa de faces:\n");
                    imprimirMapaFaces();

                    printf("1 - Continuar movimentando\n");
                    printf("2 - Sair\n");
                    printf("Escolha: ");

                    if (scanf("%d", &opcao) != 1)
                    {
                        while (getchar() != '\n');
                        opcao = 1;
                    }

                    if (opcao == 2)
                        break;
                }

                break;
            }

            case 4:
                printf("\nSaindo...\n");
                return 0;

            default:
                printf("\nOpcao invalida!\n");
                break;
        }
    }

    return 0;
}