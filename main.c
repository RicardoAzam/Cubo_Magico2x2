#include <stdio.h>
#include <stdlib.h>
#include "B_FILA.h"
#include <time.h>
#include <string.h>
#include <locale.h>

int main()
{
    setlocale(LC_ALL, "Portuguese");
    int i, j, k, x, y;
    Fila *faux = CriaFila();
    Sticker anel[8];
   
    lerCubo();
 
    if (!validarCubo()) {
        printf("\nAtencao: o cubo digitado nao tem 4 blocos de cada cor.\n");
        printf("A impressao sera feita mesmo assim, apenas como demonstracao.\n");
        imprimirCubo();
        printf("\n");
        printf("Para resolver o cubo, digite um cubo valido:\n\n");
        lerCubo();
    }
 
    imprimirCubo();

    while(1)
    {
    printf("Qual dos métodos deseja utilizar para resolver o cubo mágico?\n1- Método 1\n2- Método 2\n3- Método 3\n4-Tentar resolver\n5-Sair\n");
    scanf("%d", &x);
        switch (x)
        {
            case 1:
                //metodo1(faux);
                break;
            case 2:
                //metodo2(faux);
                break;
            case 3:
                //metodo3(faux);
            break;
            case 4:
                resolverCubo(faux, anel);
                break;
            case 5:
                exit(0);
                break;
            default:
                printf("Opção inválida!\n");
                break;
        }
    }

    return 0;
}