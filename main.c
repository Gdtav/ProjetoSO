//update a 29/10/2017, nome diferente para não apagar o que cá estava.
// Busca dados ao Config, threads e processos (com shift_lenght) funcionais.


#include <errno.h>
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
//int full_queue (Queue *);

int main() {
    signal(SIGINT, handler);
	//Inicializacao de variaveis e estruturas
    int i;
    pid_t new_doctor[num_doctors];
    //criacao da fila de pacientes para a triagem
    queue = malloc(sizeof(Queue_node));
    create_queue(queue);
    //criacao message queue
    if ((mq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0777)) == -1) {
        perror("Nao abriu message queue: ");
        exit(0);
    }
    //mapeamento das estatisticas
    mem_id = shmget(IPC_PRIVATE,sizeof(Stats),IPC_CREAT | 0777);
    stats = shmat(mem_id,NULL,0);
    //mapeamento do semaforo
    sem_id = shmget(IPC_PRIVATE,sizeof(sem_t),IPC_CREAT | 0777);
    sem = shmat(sem_id,NULL,0);
    //Criacao do pipe
    if ((mkfifo(PIPE, O_CREAT | O_EXCL | 0600)<0) && (errno != EEXIST)){
        printf("Cannot create pipe\n");
        exit(0);
    } else {
        printf("Criou o Pipe/Pipe já existe\n");
    }
    // Opens the pipe for reading
    if ((pipe_fd = open(PIPE, O_RDWR | O_NONBLOCK )) < 0) {
        printf("Nao e possivel ler do pipe\n");
        exit(0);
    } else {
        printf("Abriu pipe para leitura\n");
    }
    FILE *cfg = fopen("config.txt","r");    //ficheiro de configuracao
    pthread_t pipe_reader;
    if (cfg) {
        getconfig(cfg);
        printf("Abriu configuração\n");
        sem_init(sem,1,1);
        pthread_create(&pipe_reader,NULL,piperead,(void *)queue);
        printf("Criou semaforo e le o pipe\n");
        while (1) {
            Thread data[num_triage];
            pthread_t threads[num_triage];      //pool de threads
            for(i=0;i<num_triage;i++){
                data[i].queue = queue;
                data[i].stats = stats;
                data[i].thread_number = i;
                pthread_create(&threads[i],NULL,triage,(void *)&data[i]);
            }
            //criacao dos processos doutor
            for (i = 0; i < num_doctors; ++i) {
                new_doctor[i] = fork();
                if(new_doctor[i]==0){
                    doctor();
                    exit(0);
                }
                if(stats->triaged_patients - stats->attended_patients > mq_max)
                    temp_doctor();
            }
            for (i = 0; i < num_doctors; ++i) {
                wait(NULL);
            }
        }
    } else {
        printf("Config file not accessible. Exiting...\n");
        return -1;
    }
}


void handler(int signum){
    if (signum == SIGINT){
        printf("A eliminar todos os processos e threads...\n");
        kill(0,signum);
        pthread_kill(pthread_self(),signum);                //Falta fechar IPC's
        destroy_queue(queue);
        printf("Tudo eliminado! A sair...\n");              //MQ, pipe, MMF, etc...
        exit(0);
    }
}


Patient getpatient(char str[STR_SIZE]){
    Patient pat;
    sscanf(str,"%s %f %f %d",pat.name,&pat.triage_time,&pat.attendance_time,&pat.priority);
    if(!strcmp(pat.name,TRIAGE)) {
        num_triage = (int) pat.triage_time;
    } else {
        return pat;
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


void* triage(void *p){
    Thread *data= (Thread *) p;
    printf("Thread %d is starting!\n",data->thread_number);
    Patient pat;
    Message *msg = malloc(sizeof(Message));
        pthread_mutex_lock(&queue_mutex);
        if (!empty_queue(queue)) {
            pat = dequeue(data->queue);
            pthread_mutex_unlock(&queue_mutex);
            msg->patient = pat;
            msg->m_type = pat.priority;
            if (msgsnd(mq_id, msg, sizeof(Message), 0) == -1) {
                perror("Nao foi possivel enviar mensagem");
                pthread_exit(NULL);
            }
            pthread_mutex_lock(&stats_mutex);
            data->stats->triaged_patients++;
            printf("patients triaged: %d\n", data->stats->triaged_patients);
            pthread_mutex_unlock(&stats_mutex);
        } else {
            pthread_mutex_unlock(&queue_mutex);
            printf("Nao ha pacientes para enviar mensagem\n");
            sleep(5);
        }
    printf("Thread %d is leaving! Pacients triaged: %d\n",data->thread_number,data->stats->triaged_patients);
    pthread_exit(NULL);
}

void *piperead(void *p){
    char str[STR_SIZE];
    Patient pat;
    while(1){
        Queue *q = (Queue *) p;
        if (read(pipe_fd,str,sizeof(str)) > 0) {
            pat = getpatient(str);
            if (strcmp(pat.name,TRIAGE))
                enqueue(q,pat);
        }
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
        temp_ptr = queue->front;
        it = temp_ptr->patient;
        queue->front = queue->front->next;
        if (empty_queue (queue) == 1)
            queue->rear = NULL;
        free(temp_ptr);
        return (it);
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

/*int full_queue (Queue *queue) {
    return 0;
}*/

void temp_doctor() {
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


void doctor(){
    time_t time1 = time(NULL);
    printf("I'm doctor %d, and I will begin my shift.\n",getpid());
    Message *msg = malloc(sizeof(Message));
    int n_patients = 0;
    while(time(NULL)-time1 < shift_length) {
        sem_wait(sem);
        if (msgrcv(mq_id,msg,sizeof(msg),-10,IPC_NOWAIT) == -1 && errno != ENOMSG) {
            sem_post(sem);
            perror("Nao deu pra ler da message queue\n");
            exit(0);
        } else if (errno == ENOMSG){
            sem_post(sem);
            printf("Nao ha pacientes\n");
            sleep(1);
        }
        sem_post(sem);
        n_patients++;
    }
    printf("I'm doctor %d, and I will end my shift.\n Patients attended: %d \n",getpid(),n_patients);
}