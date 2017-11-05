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

int num_doctors,num_triage,mq_max,shift_length,triaged_pacients=0;
pthread_cond_t triage_threshold_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t triage_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct queue * Queue;

typedef struct pacient{
    int arrival_number;
    char name[50];
    int triage_time;
    int priority;
    int attendance_time;
}Pacient;

typedef struct queue{
    Pacient* pacient;
    Queue next;
}Queue_node;

typedef struct thread_stuff{
    Queue *queue;
    int thread_number;
}Stuff;

void doctor(){
    time_t time1 = time(NULL);
    time_t time2,time3;
    printf("I'm doctor %d, and I will begin my shift.\n",getpid());
    do{
	time2 = time(NULL);
        time3 = difftime(time2,time1);
    }while(time3<shift_length);
    printf("I'm doctor %d, and I will end my shift.\n",getpid());
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
    Stuff *data = (Stuff*)t;

    printf("Thread %d is starting!\n",data->thread_number);
    pthread_mutex_lock(&triage_mutex);
    triaged_pacients++;
    pthread_mutex_unlock(&triage_mutex);

    printf("Thread %d is leaving! Pacients triaged: %d\n",data->thread_number,triaged_pacients);
    pthread_exit(NULL);
}


int main() {
    int i;
    int status;
    pid_t new_doctor;
    Queue queue = malloc(sizeof(Queue));
    FILE *cfg = fopen("config.txt","r");
    getconfig(cfg);
    int n=num_doctors;
    Stuff thread[num_triage];
    pthread_t threads[num_triage];
    if (cfg) {
	for(i=0;i<num_triage;i++){
	    thread[i].queue = &queue;
	    thread[i].thread_number = i;
	    pthread_create(&threads[i],NULL,triage,(void*)&thread[i]);
	}
	while(n>0){
	    new_doctor = fork();
	    n--;
	    if(new_doctor==0){
	    doctor();
	    exit(0);
	    }
	}
	for(i=0;i<num_triage;i++){
	    pthread_join(threads[i],NULL);
	}
	for(i=0;i<num_doctors;i++){
	    wait(&status);
	}
	
    } else {
        printf("Config file not accessible. Exiting...");
        return 1;
    }
    return 0;
}
