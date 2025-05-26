#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <unistd.h>
#include <math.h>

sem_t slotCheio, slotVazio, contador; 
sem_t mutexGeral;

int* canal;
int N;
int M;
int n_analisados = 0, numeros_no_canal = 0;


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
        printf("%d ",canal[i]);
    }
    printf("]\n");
}

void Insere(int num, int M){
    static int in;
    sem_wait(&slotVazio);
    sem_wait(&mutexGeral);

    canal[in] = num;
    in = (in+1)%M;
    if(num != -1){
        sem_wait(&contador);
        numeros_no_canal++;
        sem_post(&contador);
    }
    printf("Inserido %d\n", num);
    printCanal(canal,M);

    sem_post(&mutexGeral);
    sem_post(&slotCheio);
}

int verificaPrimo(int id, int M, threadConsome *args){
    static int out;
    int validar_num; 
    sem_wait(&slotCheio);
    sem_wait(&mutexGeral);
    validar_num = canal[out];
    if(validar_num == -1) {
        sem_wait(&contador);
        if (numeros_no_canal == 0) {
            sem_post(&mutexGeral);
            sem_post(&slotCheio);
            sem_post(&contador);
            return 1;
        }
        sem_post(&contador);
    }
    printf("Numero %d sera verificado pela consumidora %d\n", validar_num, id);
    canal[out] = 0;
    out = (out+1)%M;
    if(ehPrimo(validar_num)){
        args->quantoConsumiu+=1;
        printf("%d eh primo\n", validar_num);
    }
    printCanal(canal,M);

    sem_wait(&contador);
    numeros_no_canal--;
    sem_post(&contador);

    sem_post(&mutexGeral);
    sem_post(&slotVazio);
    return 0;
}


void *produtor(void *arg){
    int nthreads = (int) arg;
    for (int i = 0; i < N; i++) {
        Insere(i, M);
    }
    while(1){
        sem_wait(&contador);
        if(numeros_no_canal == 0){ //nao tem mais numeros no canal
            sem_post(&contador);
            break;
        }
        sem_post(&contador);
    }
    for(int i = 0; i<nthreads;i++){
        Insere(-1,M);
    }

    pthread_exit(NULL);
    return 0;
}

void *consumidor(void *arg){
    threadConsome * args = (threadConsome *)(arg);
    printf("estou na thread %d\n", args->id);
    int result = 0;
    //free(arg);
    while(n_analisados<N){
        result += verificaPrimo(args->id,M,args);
        printf("result %d \n", result);
        if(result) break;
    }
    printf("saindo da thread %d\n", args->id);
    pthread_exit(NULL);
    return 0;
}

int main(int argc, char * argv[]){
    int nthreads;
    int thread_vencedora;
    int valor_max_analisado=0, total_de_primos = 0;
    if(argc<4){
        printf("ERRO----\nPorfavor insira na linha de comando algo como %s <ate que numero analisar> <tamanho do canal> <numero de threads> \n",argv[0]);
        return 1;
    }
    N = atoi(argv[1]);
    M = atoi(argv[2]);
    nthreads = atoi(argv[3]);
    canal = (int * )calloc(M,sizeof(int));
    if(N<nthreads)nthreads=N;
    pthread_t tid[nthreads+1]; //mais 1 porque teremos 1 threads que irá inserir e o usuario nos dara o numero de threads que serão consumidoras
    threadConsome * args_consome;

    args_consome = (threadConsome *) malloc(sizeof(threadConsome)*nthreads);

    sem_init(&mutexGeral, 0, 1);
    sem_init(&slotCheio, 0, 0);
    sem_init(&slotVazio, 0, M);

    for(int i = 0; i<nthreads;i++){
        (args_consome+i)->id=i;
        (args_consome+i)->quantoConsumiu = 0;
        if(pthread_create(&tid[i],NULL,consumidor, (void *) (args_consome+i))){
            printf("ERRO----\nErro ao criar threads\n");
            return 2;
        }
    }
    if(pthread_create(&tid[nthreads],NULL,produtor, (void *) nthreads)){
        printf("ERRO-----\nErro ao criar thread\n");
        return 3;
    }

    for(int i = 0; i<nthreads+1;i++){
        if(pthread_join(tid[i], NULL)){
            printf("ERRO------\nErro ao receber threads\n");
            return 4;
        }
        if(i<nthreads){
            printf("a thread %d consumiu %d\n", (args_consome+i)->id, (args_consome+i)->quantoConsumiu);
            total_de_primos += (args_consome+i)->quantoConsumiu;
            if(valor_max_analisado<(args_consome+i)->quantoConsumiu){
                valor_max_analisado = (args_consome+i)->quantoConsumiu;
                thread_vencedora = (args_consome+i)->id;
            }
        }
    }
    printf("Entao ao final de nossa analise a thread vencedora eh: %d descobrindo %d primos de %d\n", thread_vencedora, valor_max_analisado, total_de_primos);
    free(args_consome);
    return 0;
}