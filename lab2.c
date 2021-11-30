#include <stdio.h>
#include <stdlib.h>
#include "timer.h"
#include <time.h>

typedef struct {
   int size, skip, take; 
   int **matriz1;
   int **matriz2;
   int **resultado;
} t_Args;

void MultiplicacaoSequencial(int size, int **matriz1, int **matriz2, int **result) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            result[i][j] = 0;
            for (int k = 0; k < size; k++) {
                result[i][j] += matriz1[i][k] * matriz2[k][j];
            }
        }
    }
}

void *Multiplica(void *arg) 
{
    t_Args *args = (t_Args *) arg;

    // Determinamos em qual coluna devemos começar a multiplicar a matriz
    int colunaInicio = args->skip == 0 ? 0 : args->skip / args->size;

    // Confuso, mas aqui nós determinamos a posição de inicio na linha da matriz. 
    int linhaInicio = args->skip == 0 || (args->skip % args->size) == 0 ? 0 : args->size - (args->skip % args->size);


    // Itera na quantidade de itens que é trabalho da thread
    for(int o = 0; o < args->take; o++) {
        int sum = 0;
        for (int k = 0; k < args->size; k++) {
            sum += args->matriz1[colunaInicio][k] * args->matriz2[k][linhaInicio];
        }
        args->resultado[colunaInicio][linhaInicio] += sum;

        // Ajusta posição para a próxima posição da matriz a ser calculada
        if(linhaInicio < args->size - 1) {
            linhaInicio++;
        } else {
            colunaInicio++;
            linhaInicio = 0;
        }
    }
}

void MultiplicaoParalela(int size, int numThreads, t_Args *arg) {

    t_Args *arg_thread;

    pthread_t tid_sistema[numThreads]; //identificadores das threads no sistema

    // Determinamos o número de operações que cada thread irá realizar
    int numOperacoes = size * size;

    int numOperacoesThread = numOperacoes / numThreads;

    int excessoOperacoes = numOperacoes % numThreads;

    for (int o = 0; o < numThreads; o++) {

        arg_thread = malloc(sizeof(t_Args));

        arg_thread->matriz1 = arg->matriz1;
        arg_thread->matriz2 = arg->matriz2;
        arg_thread->resultado = arg->resultado;
        arg_thread->size = arg->size;

        // Solução "criativa" para tentar distribuir o resto da divisão de operações por threads
        // As primeiras threads recebem 1 operação de carga extra, e o skip é calculado de forma a levar em consideração essa carga extra
        arg_thread->skip = (numOperacoesThread * o) + (o < excessoOperacoes ? o : excessoOperacoes);
        arg_thread->take = o < excessoOperacoes ? numOperacoesThread + 1 : numOperacoesThread;
        
        if (pthread_create(&tid_sistema[o], NULL, Multiplica, (void*)arg_thread)) {
                printf("--ERRO: pthread_create()\n"); exit(-1);
        }
    }

    // Espera as threads terminarem
    for (int o =0; o<numThreads; o++) {
        if (pthread_join(tid_sistema[o], NULL)) {
            printf("--ERRO: pthread_join() \n"); 
            exit(-1); 
        } 
    }
}
int ChecaCorretude(int size, int **resultado1, int **resultado2) {

    int iguais = 1;

    for(int o = 0; o < size; o++) {
        for(int i = 0; i < size; i++) {
            if(resultado1[o][i] != resultado2[o][i]) {
                printf("\nPosições: %d %d %d %d", o, i, resultado1[o][i] , resultado2[o][i] );
                iguais = 0;
            }
        }
    }

    if(iguais) {
        printf("\nValidação de corretude passou. Matrizes são iguais\n");
    } else {
        printf("\nValidação falhou\n");
    }

    return iguais;
}
 
int main(void) {
    double start, finish, elapsed;

    // Inicializa matrizes com valores aleatorios

    printf("\nEntre com a dimensão das matrizes quadradas:");

    int size;

    scanf("%d", &size);

    printf("\nAgora entre com o número de threads:");

    int threads;

    scanf("%d", &threads);

    if(size % threads > 0) {
        printf("\nErro: O número de threads deve poder dividir a dimensão das matrizes\n");
        exit(1);
    }

    int **matriz1;
 
    int **matriz2;

    matriz1 = (int **)malloc(size * sizeof(int*));
    for(int i = 0; i < size; i++) matriz1[i] = (int *)malloc(size * sizeof(int));

    matriz2 = (int **)malloc(size * sizeof(int*));
    for(int i = 0; i < size; i++) matriz2[i] = (int *)malloc(size * sizeof(int));

    srand(time(NULL));

    for(int o = 0; o<size; o++)
        for(int i = 0; i<size; i++) {
            matriz1[o][i] = rand();
            matriz2[o][i] = rand();
        };
    
    // Calcula de forma sequencial

    int **resultadoSequencial;
    resultadoSequencial = (int **)malloc(size * sizeof(int*));
    for(int i = 0; i < size; i++) resultadoSequencial[i] = (int *)malloc(size * sizeof(int));

    GET_TIME(start);
    MultiplicacaoSequencial(size, matriz1, matriz2, resultadoSequencial);
    GET_TIME(finish);
    elapsed = finish - start;
    printf("Sequencial levou %e segundos\n", elapsed);
    // Calcula de forma paralela

    // Popula os dados da matriz em um argumento para utilização nas threads
    // É feito fora do método para não atrapalhar a contagem da performance

    t_Args *arg;

    arg = malloc(sizeof(t_Args));

    arg->size = size;

    arg->matriz1 = (int **)malloc(size * sizeof(int*));
    for(int i = 0; i < size; i++) arg->matriz1[i] = (int *)malloc(size * sizeof(int));

    arg->matriz2 = (int **)malloc(size * sizeof(int*));
    for(int i = 0; i < size; i++) arg->matriz2[i] = (int *)malloc(size * sizeof(int));

    arg->resultado = (int **)malloc(size * sizeof(int*));
    for(int i = 0; i < size; i++) arg->resultado[i] = (int *)malloc(size * sizeof(int));



    for(int o = 0; o<size; o++)
        for(int i = 0; i<size; i++) {
            arg->matriz1[o][i] = matriz1[o][i];
            arg->matriz2[o][i] = matriz2[o][i];
        };

    GET_TIME(start);
    MultiplicaoParalela(size, threads, arg);
    GET_TIME(finish);
    elapsed = finish - start;
    printf("Paralelo levou %e segundos\n", elapsed);

    ChecaCorretude(size, resultadoSequencial, arg->resultado);
    
    return 0;
}

