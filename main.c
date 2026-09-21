#include <stdio.h>
#include <stdlib.h>
#include "B_FILA.h"
#include <locale.h>

int main(void)
{
    setlocale(LC_ALL, "Portuguese");

    lerCubo();

    if (!validarCubo())
    {
        printf("\nAtencao: o cubo digitado nao tem 4 blocos de cada cor.\n");
        printf("A impressao sera feita mesmo assim.\n\n");
    }

    int opcaoPrincipal;

    while (1)
    {
        imprimirCubo();

        printf("1- Visualizar metodos de resolucao\n");
        printf("2- Tentar mexer no cubo\n");
        printf("3- Sair\n");
        printf("Opcao: ");
        scanf("%d", &opcaoPrincipal);

        switch (opcaoPrincipal)
        {
            case 1:
                printf("\nEsta funcionalidade ainda sera implementada.\n");
                break;

            case 2:
            {
                int continuar = 1;

                while (continuar == 1)
                {
                    imprimirCubo();
                    imprimirMapaFaces();

                    int face;
                    int sentido;
                    char faixa;

                    escolherMovimento(&face, &faixa, &sentido);

                    if (face != -1 && faixa != '\0' && sentido != -1)
                    {
                        moverCamada(face, faixa, sentido);
                        imprimirCubo();
                    }

                    printf("1- Continuar movimentando\n");
                    printf("2- Sair\n");
                    printf("Opcao: ");
                    scanf("%d", &continuar);

                    if (continuar != 1)
                        continuar = 0;
                }

                break;
            }

            case 3:
                return 0;

            default:
                printf("Opcao invalida!\n");
                break;
        }
    }
}
