#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>

#define PRODUTORES 10
#define CONSUMIDORES 5
#define MAX (PRODUTORES+CONSUMIDORES)

sem_t slotCheio, slotVazio; 
sem_t mutexGeral;

int* canal;
int N;
int M;
int n_analisados = 0;
typedef struct 
{
    int id;
}threadInsere;

typedef struct 
{
    int id;
    int quantoConsumiu;
}threadConsome;


int ehPrimo(long long int n) {
    int i;
    if (n<=1) return 0;
    if (n==2) return 1;
    if (n%2==0) return 0;
    for (i=3; i<sqrt(n)+1; i+=2)
        if(n%i==0) return 0;
    return 1;
}
void printCanal(int canal[], int M){
    printf("[");
    for(int i = 0;i<M;i++){
        printf("%d ");
    }
    printf("]\n");
}

void Insere(int num, int id,int M){
    static int in;
    sem_wait(&slotVazio);
    sem_wait(&mutexGeral);

    canal[in] = num;
    in = (in+1)%M;
    printf("Inserido %d\n", num);
    printCanal(canal,M);

    sem_post(&mutexGeral);
    sem_post(&slotCheio);
}

int verificaPrimo(int id, int M){
    static int out;
    int validar_num; 
    sem_wait(&slotCheio);
    sem_wait(&mutexGeral);
    validar_num = canal[out];
    printf("Numero %d sera verificado pela consumidora %d\n", validar_num, id);
    canal[out] = 0;
    if(n_analisados>=N) return 1;
    ehPrimo(validar_num);
    out = (out+1)%M;
    printCanal(canal,M);
    sem_post(&mutexGeral);
    sem_post(&slotVazio);
    return 0;
}


void *produtor(void *arg){
    threadInsere * args = (threadInsere *)(arg); 
    free(arg);
    while(n_analisados<N){
        sleep(1);
        Insere(n_analisados, args->id, M);
        n_analisados+=1;
    }
    pthread_exit(NULL);
}

void *consumidor(void *arg){
    threadConsome * args = (threadConsome *)(arg);
    int result;
    free(arg);
    while(1){
        result = verificaPrimo(args->id,M);
        args->quantoConsumiu +=1;
        if(result) break;
    }

    pthread_exit(NULL);
}

int main(int argc, char * argv[]){
    int nthreads;
    int thread_vencedora;
    int valor_max_analisado=0;
    if(argc<4){
        printf("ERRO----\nPorfavor insira na linha de comando algo como %s<ate que numero analisar> <tamanho do canal> <numero de threads>\n");
        return 1;
    }
    N = atoi(argv[1]);
    M = atoi(argv[2]);
    nthreads = atoi(argv[3]);
    if(N<nthreads)nthreads=N;
    pthread_t tid[nthreads+1]; //mais 1 porque teremos 1 threads que irá inserir e o usuario nos dara o numero de threads que serão consumidoras
    threadInsere* args_insere;
    threadConsome * args_consome;


    sem_init(&mutexGeral, 0, 1);
    sem_init(&slotCheio, 0, 0);
    sem_init(&slotVazio, 0, N);

    canal = (int*) malloc(sizeof(int)*M);
    for(int i = 0; i<nthreads;i++){
        (args_consome+i)->id=i;
        (args_consome+i)->quantoConsumiu = 0;
        if(pthread_create(&tid[i],NULL,consumidor, (void *) (args_consome+i))){
            printf("ERRO----\nErro ao criar threads\n");
            return 2;
        }
    }
    if(pthread_create(&tid[nthreads+1],NULL,produtor, (void *) args_insere)){
        printf("ERRO-----\nErro ao criar thread\n");
        return 3;
    }

    for(int i = 0; i<nthreads+1;i++){
        if(pthread_join(tid[i], NULL)){
            printf("ERRO------\nErro ao receber threads\n");
            return 4;
        }
        if(valor_max_analisado<(args_consome+i)->quantoConsumiu)
            valor_max_analisado = (args_consome+i)->quantoConsumiu;
            thread_vencedora = (args_consome+i)->id;
    }
    printf("Entao ao final de nossa analise a thread vencedora eh: %d\n", thread_vencedora);
    free(args_consome);
    free(args_insere);
    return 0;
}