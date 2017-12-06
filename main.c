//update a 29/10/2017, nome diferente para não apagar o que cá estava.
// Busca dados ao Config, threads e processos (com shift_lenght) funcionais.


#include "structs.h"

void handler(int );
Patient getpatient(char []);
void getconfig(FILE* );
void* triage(void *);
void* piperead(void *);
void temp_doctor();
void doctor();
void create_queue(Queue *);
void enqueue(Queue *, Patient);
Patient dequeue(Queue *);
void destroy_queue (Queue *);
int empty_queue(Queue *);
int full_queue (Queue *);

int main() {
    signal(SIGINT, handler);
	//Inicializacao de variaveis e estruturas
    int i, n;
    pid_t new_doctor;
    //criacao da fila de pacientes para a triagem
    create_queue(queue);
    //criacao message queue
    mq_id = msgget(IPC_PRIVATE, IPC_CREAT);
    //mapeamento das estatisticas
    mem_id = shmget(IPC_PRIVATE,sizeof(Stats),IPC_CREAT | 0777);
    Stats *stats = shmat(mem_id,NULL,0);
    //mapeamento do semaforo
    sem_id = shmget(IPC_PRIVATE,sizeof(sem_t),IPC_CREAT | 0777);
    sem = shmat(sem_id,NULL,0);
    if ((mkfifo(PIPE, O_CREAT|O_EXCL|0600)<0)){                          //Abertura do pipe
        printf("Cannot create pipe");
        exit(0);
    }
    // Opens the pipe for reading
    if ((pipe_fd = open(PIPE, O_RDONLY)) < 0) {
        printf("Cannot open pipe for reading");
        exit(0);
    }

    FILE *cfg = fopen("config.txt","r");    //ficheiro de configuracao
    getconfig(cfg);
    Thread thread[num_triage];
    pthread_t threads[num_triage];          //pool de threads
    pthread_t pipe_reader;
    if (cfg) {
        sem_init(sem,1,1);
        pthread_create(&pipe_reader,NULL,piperead,(void *)queue);
        for(i=0;i<num_triage;i++){
            thread[i].queue = queue;
            thread[i].thread_number = i;
            thread[i].stats = stats;
            pthread_create(&threads[i],NULL,triage,(void*)&thread[i]);
        }
        while (1) {     
            n=num_doctors;
            //criacao dos processos doutor
            while(n>0){
                new_doctor = fork();
                n--;
                if(new_doctor==0){
                doctor(stats);
                exit(0);
                }
                if(stats->triaged_patients - stats->attended_patients > mq_max)
                    temp_doctor(stats);
            }

        }
    } else {
        printf("Config file not accessible. Exiting...");
        return -1;
    }
    return 0;
}


void handler(int signum){
    if (signum == SIGINT){
        printf("A eliminar todos os processos e threads...\n");
        kill(0,signum);
        pthread_kill(pthread_self(),signum);                //Falta fechar IPC's
        printf("Tudo eliminado! A sair...\n");              //MQ, pipe, MMF, etc...
        exit(0);
    }
}


Patient getpatient(char str[STR_SIZE]){
    Patient pat;
    sscanf(str,"%s %f %f %d",pat.name,&pat.triage_time,&pat.attendance_time,&pat.priority);

    return pat;
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
    Patient pat;
    Message *msg = malloc(sizeof(Message));
    while(1){
        pthread_mutex_lock(&queue_mutex);
        pat = dequeue(data->queue);
        pthread_mutex_unlock(&queue_mutex);
        msg->patient = pat;
        msg->m_type = pat.priority;
        pthread_mutex_lock(&stats_mutex);
        msgsnd(mq_id,msg,sizeof(Message),0);
        data->stats->triaged_patients++;
        printf("patients triaged: %d\n", data->stats->triaged_patients);
        pthread_mutex_unlock(&stats_mutex);
    }
    

    //printf("Thread %d is leaving! Pacients triaged: %d\n",data->thread_number,data->stats->triaged_patients);
    //pthread_exit(NULL);
}

void *piperead(void *p){
    char str[STR_SIZE];
    while(1){
        Queue *q = (Queue *) p;
        read(pipe_fd,str,sizeof(str));       
        enqueue(q,getpatient(str)); 
    }
}

void create_queue(Queue *queue){
    queue->front = NULL;
    queue->rear = NULL;
}

void enqueue(Queue *queue, Patient pat){
    q_ptr temp_ptr;
    temp_ptr = (q_ptr) malloc (sizeof (Queue_node));
    if (temp_ptr != NULL) {
        temp_ptr->patient = pat;
        temp_ptr->next = NULL;
        if (empty_queue (queue) == 1)
            queue->front = temp_ptr;
        else 
            queue->rear->next = temp_ptr;
        queue->rear = temp_ptr; 
    }
}

Patient dequeue(Queue *queue){
    q_ptr temp_ptr;
    Patient it;
    if (empty_queue(queue) == 0) {
        temp_ptr = queue->front;
        it = temp_ptr->patient;
        queue->front = queue->front->next;
        if (empty_queue (queue) == 1)
            queue->rear = NULL;
        free (temp_ptr);
        return (it); 
    }
}

void destroy_queue (Queue *queue) {
    q_ptr temp_ptr;
    while (empty_queue (queue) == 0) {
        temp_ptr = queue->front;
        queue->front = queue->front->next;
        free(temp_ptr);
    }
    queue->rear = NULL;
}


int empty_queue(Queue*queue) {
    return (queue->front == NULL ? 1 : 0);
}

int full_queue (Queue *queue) {
    return 0;
}




void temp_doctor(Stats *stats) {
    printf("Temporary doctor entering\n");
    Message *msg = malloc(sizeof(Message));
    int n_patients = 0;
    while(stats->triaged_patients - stats->attended_patients > 0.8 * mq_max){
        sem_wait(sem);
        msgrcv(mq_id,msg,sizeof(msg),-10,0);
        stats->attended_patients++;
        sem_post(sem);
        n_patients++;
    }
    printf("Temporary doctor leaving. Patients attended: %d \n", n_patients);
}



void doctor(Stats* stats){
    //time_t time1 = time(NULL);
    //time_t time2,time3;
    clock_t time_;
    printf("I'm doctor %d, and I will begin my shift.\n",getpid());
    Message *msg = malloc(sizeof(Message));
    time_ = clock();
    int n_patients = 0;
    do{
        sem_wait(sem);
        msgrcv(mq_id,msg,sizeof(msg),-10,0);
        sem_post(sem);
        n_patients++;
    } while((clock() - time_)/CLOCKS_PER_SEC < shift_length);
    printf("I'm doctor %d, and I will end my shift.\n Patients attended: %d \n",getpid(),n_patients);
}