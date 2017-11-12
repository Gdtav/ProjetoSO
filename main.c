//update a 29/10/2017, nome diferente para não apagar o que cá estava.
// Busca dados ao Config, threads e processos (com shift_lenght) funcionais.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/shm.h>
#include <signal.h>
#include "structs.h"

pthread_cond_t triage_threshold_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t triage_mutex = PTHREAD_MUTEX_INITIALIZER;

void handler(int signum){
    if (signum == SIGINT){
        printf("A eliminar todos os processos e threads...\n");
        kill(0,signum);
        pthread_kill(pthread_self(),signum);
        printf("Tudo eliminado! A sair...\n");
        exit(0);
    }
}

void getconfig(FILE* cfg){
    char linha[50];
    char copia[20];

    fgets(linha,50,cfg);
    strcpy(copia,linha+7);
    num_triage = atoi(copia);
    fgets(linha+9,50,cfg);
    strcpy(copia,linha+17);
    num_doctors = atoi(copia);
    fgets(linha+20,50,cfg);
    strcpy(copia,linha+33);
    shift_length = atoi(copia);
    fgets(linha+35,50,cfg);
    strcpy(copia,linha+42);
    mq_max = atoi(copia);
}        

void* triage(void *t){
    Thread *data = (Thread *)t;

    printf("Thread %d is starting!\n",data->thread_number);
    pthread_mutex_lock(&triage_mutex);
    data->stats->triaged_patients++;
    pthread_mutex_unlock(&triage_mutex);

    printf("Thread %d is leaving! Pacients triaged: %d\n",data->thread_number,data->stats->triaged_patients);
    pthread_exit(NULL);
}

void doctor(Stats *stats){
    time_t time1 = time(NULL);
    time_t time2,time3;
    printf("I'm doctor %d, and I will begin my shift.\n",getpid());
    do{
    	time2 = time(NULL);
    	/*for (i = 0; i < Patients; ++i)
    	{
    		//process Patient
    		stats->attended_patients++;
    	}*/
        time3 = difftime(time2,time1);
    } while(time3<shift_length);
    stats->attended_patients++;
    printf("I'm doctor %d, and I will end my shift.\n Patients attended: %d \n",getpid(),stats->attended_patients);
}

int main() {
	//Inicializacao de variaveis e estruturas
    signal(SIGINT, handler);
    int i, n;
    int status;
    pid_t new_doctor;
    Queue queue = malloc(sizeof(Queue));
    int mem_id = shmget(IPC_PRIVATE,sizeof(Stats),IPC_CREAT | 0777);
    Stats *stats = shmat(mem_id,NULL,0);
    FILE *cfg = fopen("config.txt","r");
    getconfig(cfg);
    Thread thread[num_triage];
    pthread_t threads[num_triage]; //pool de threads
    if (cfg) {
        while (1) {
            n=num_doctors;
            for(i=0;i<num_triage;i++){
                thread[i].queue = &queue;
                thread[i].thread_number = i;
                thread[i].stats = stats;
                pthread_create(&threads[i],NULL,triage,(void*)&thread[i]);
            }
            //criacao dos processos doutor
            while(n>0){
                new_doctor = fork();
                n--;
                if(new_doctor==0){
                doctor(stats);
                exit(0);
                }
            }
            for(i=0;i<num_triage;i++){
                pthread_join(threads[i],NULL);
            }
            for(i=0;i<num_doctors;i++){
                wait(&status);
            }
        }
    } else {
        printf("Config file not accessible. Exiting...");
        return 1;
    }
    return 0;
}
