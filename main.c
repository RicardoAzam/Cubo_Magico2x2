#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

#include "cubo.h"
#include "busca.h"

int main(void)
{
    setlocale(LC_ALL, "Portuguese");

    lerCubo();

    if (!validarCubo())
    {
        printf("\nAtencao: o cubo digitado nao tem 4 blocos de cada cor.\n");
        printf("A impressao sera feita mesmo assim.\n\n");
    }
    else
    {
        /* Guarda o estado exatamente como foi inserido. */
        salvarEstadoInicial();
    }

    int opcaoPrincipal;

    while (1)
    {
        imprimirCubo();

        printf("1- Visualizar metodos de resolucao\n");
        printf("2- Tentar mexer no cubo\n");
        printf("3- Resolver em profundidade\n");
        printf("4- Resolver em largura\n");
        printf("5- Resolver por camadas\n");
        printf("6- Comparar os 3 metodos\n");
        printf("7- Sair\n");
        printf("Opcao: ");
        scanf("%d", &opcaoPrincipal);

        switch (opcaoPrincipal)
        {
            case 1:
                printf("\nMetodos de resolucao do projeto:\n");
                printf("  1 - Busca em profundidade\n");
                printf("  2 - Busca em largura\n");
                printf("  3 - Busca por camadas superiores: Procura-se um lado com a camada superior correta e apartir dele tenta resolver o cubo\n");
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

                        /*
                         * A partir do primeiro movimento manual, o estado
                         * atual passa a ser a referencia da comparacao.
                         * Salvar em cada movimento garante que, ao sair do
                         * modo de jogo, o ultimo estado esteja preservado.
                         */
                        salvarEstadoComparacao();
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
            {
                char letra;
                int faceInicial = -1;

                printf("\n=== Busca da solucao em profundidade ===\n");
                printf("Escolha o lado que sera priorizado primeiro (U, D, F, B, L, R): ");
                scanf(" %c", &letra);

                switch (letra)
                {
                    case 'U': case 'u': faceInicial = U; break;
                    case 'D': case 'd': faceInicial = D; break;
                    case 'F': case 'f': faceInicial = F; break;
                    case 'B': case 'b': faceInicial = B; break;
                    case 'L': case 'l': faceInicial = L; break;
                    case 'R': case 'r': faceInicial = R; break;
                    default:
                        printf("Lado invalido.\n");
                        break;
                }

                if (faceInicial != -1)
                {
                    int resultado = resolverEmProfundidade(faceInicial);

                    if (resultado == -1)
                        printf("A busca nao pode ser iniciada com este estado do cubo.\n");
                    else if (resultado == 0 && !estadoResolvido())
                        printf("Tente um cubo valido ou um embaralhamento menor.\n");
                    else if (resultado >= 0)
                    {
                        imprimirCubo();
                        printf("\nO cubo foi deixado no estado final encontrado pela busca.\n");
                    }
                }

                break;
            }

            case 4:
            {
                printf("\n=== Busca da solucao em largura ===\n");
                int resultado = resolverEmLargura();

                if (resultado == -1)
                {
                    printf("A busca em largura nao pode ser iniciada com este estado do cubo.\n");
                }
                else if (resultado == 0 && !estadoResolvido())
                {
                    printf("A busca foi encerrada sem encontrar uma solucao dentro dos limites configurados.\n");
                }
                else
                {
                    imprimirCubo();
                    printf("\nO cubo foi deixado no estado final encontrado pela busca.\n");
                }

                break;
            }

            case 5:
            {
                printf("\n=== Busca da solucao por camadas ===\n");
                int resultado = resolverPorCamadas();

                if (resultado == -1)
                {
                    printf("A busca por camadas nao pode ser iniciada com este estado do cubo.\n");
                }
                else if (resultado == 0 && !estadoResolvido())
                {
                    printf("A busca por camadas nao encontrou uma solucao dentro dos limites configurados.\n");
                }
                else
                {
                    imprimirCubo();
                    printf("\nO cubo foi deixado no estado final encontrado pela busca por camadas.\n");
                }

                break;
            }

            case 6:
            {
                if (!existeEstadoComparacao())
                {
                    printf("\nNao existe um estado valido salvo para comparacao.\n");
                    printf("Insira um cubo valido antes de iniciar a comparacao.\n");
                    break;
                }

                printf("\n==================================================\n");
                printf("        COMPARACAO DOS 3 METODOS\n");
                printf("==================================================\n");
                printf("Todos os metodos serao executados a partir do MESMO estado salvo.\n");
                printf("\n");

                /* 1 - Profundidade */
                restaurarEstadoComparacao();
                printf("\n>>> 1. BUSCA EM PROFUNDIDADE\n");
                printf("--------------------------------------------------\n");
                int resultadoProfundidade = resolverEmProfundidade(U);
                (void)resultadoProfundidade;

                /* 2 - Largura */
                restaurarEstadoComparacao();
                printf("\n>>> 2. BUSCA EM LARGURA\n");
                printf("--------------------------------------------------\n");
                int resultadoLargura = resolverEmLargura();
                (void)resultadoLargura;

                /* 3 - Camadas */
                restaurarEstadoComparacao();
                printf("\n>>> 3. BUSCA POR CAMADAS\n");
                printf("--------------------------------------------------\n");
                int resultadoCamadas = resolverPorCamadas();
                (void)resultadoCamadas;

                /* Mantem o estado original da comparacao intacto para uma
                 * nova rodada e para permitir repetir o teste. */
                restaurarEstadoComparacao();

                printf("\n==================================================\n");
                printf("Comparacao finalizada.\n");
                printf("Os relatorios acima mostram tempo, nos/estados explorados\n");
                printf("e quantidade de movimentos encontrada por cada metodo.\n");
                printf("O cubo foi restaurado ao estado usado na comparacao.\n");
                printf("==================================================\n");
                break;
            }

            case 7:
                return 0;

            default:
                printf("Opcao invalida!\n");
                break;
        }
    }
}
