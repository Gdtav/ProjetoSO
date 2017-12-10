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
//int full_queue (Queue *);
void cria_estatisticas(Stats *);

int main() {
    signal(SIGINT, handler);
	//Inicializacao de variaveis e estruturas
    int i;
    pid_t new_doctor[num_doctors];
    //criacao da fila de pacientes para a triagem
    queue = malloc(sizeof(Queue_node));
    create_queue(queue);
    //criacao message queue
    if ((mq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0666)) == -1) {
        perror("Nao abriu message queue: ");
        exit(0);
    }
    //mapeamento das estatisticas
    if ((mem_id = shmget(IPC_PRIVATE,sizeof(Stats),IPC_CREAT | 0666)) == -1){
        perror("Nao abriu memoria partilhada");
        exit(0);
    }
    if ((stats = shmat(mem_id,NULL,0)) == (void *) -1){
        printf("Nao fez attach das estatisticas\n");
        exit(0);
    }
    //mapeamento do semaforo stats
    if ((sstats_id = shmget(IPC_PRIVATE,sizeof(sem_t),IPC_CREAT | 0666)) == -1){
        perror("Nao abriu semaforo 1");
        exit(0);
    }
    if ((sstats = shmat(sstats_id,NULL,0)) == (void*) -1){
        printf("nao fez attach do semaforo 1\n");
        exit(0);
    }
    //semaforo log
    if ((slog_id = shmget(IPC_PRIVATE,sizeof(sem_t),IPC_CREAT | 0666)) == -1){
        perror("Nao abriu semaforo 2");
        exit(0);
    }
    if ((slog = shmat(slog_id,NULL,0)) == (void*) -1){
        printf("nao fez attach do semaforo 2\n");
        exit(0);
    }
    //Criacao do pipe
    if (mkfifo(PIPE, O_CREAT | 0600) == -1){
        perror("Nao e possivel criar pipe");
        exit(0);
    }
    // Opens the pipe for reading/writing
    if ((pipe_fd = open(PIPE, O_RDWR)) == -1) {
        perror("Nao e possivel ler do pipe");
        exit(0);
    }
    //opens log file for reading/writing
    if ((log_fd = open(LOG,O_CREAT | O_RDWR | O_TRUNC, 0600)) == -1){
        perror("Nao e possivel abrir o log");
        exit(0);
    }
    if(lseek(log_fd,LOG_SIZE,SEEK_SET) < 0){
        perror("Erro lseek");
        exit(0);
    }
    if(write(log_fd,"",1) != 1){
        perror("Nao e possivel escrever no log");
        close(log_fd);
        exit(0);
    }
    if ((log_map = mmap(NULL,LOG_SIZE,PROT_READ | PROT_WRITE,MAP_SHARED,log_fd,0)) == MAP_FAILED){
        perror("Erro a mapear log file");
        exit(0);
    }
    //opens config file and reads it if successful
    FILE *cfg = fopen("config.txt","r");    //ficheiro de configuracao
    if (cfg) {
        getconfig(cfg);
        fclose(cfg);
        if (sem_init(sstats,1,1) == -1) {
            perror("Nao iniciou o semaforo 1");
            exit(0);
        }
        if (sem_init(slog,1,1) == -1) {
            perror("Nao iniciou o semaforo 2");
            exit(0);
        }
        pthread_t pipe_reader;
        pthread_create(&pipe_reader,NULL,piperead,(void *)queue);
        while (1) {
            signal(SIGUSR1,handler);
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
                //chama um doutor temporario se a fila de mensagens ultrapassar o maximo
                if(stats->triaged_patients - stats->attended_patients > mq_max)
                    temp_doctor();
            }
            for (i = 0; i < num_doctors; i++){
                wait(NULL);
            }
        }
    } else {
        printf("Ficheiro de configuracao nao disponivel. A sair...\n");
        return -1;
    }
}


void handler(int signum){
    if (signum == SIGINT){
        printf("A eliminar todos os processos e threads...\n");
        kill(0,signum);
        pthread_kill(pthread_self(),signum);                //Falta fechar IPC's
        destroy_queue(queue);
        msgctl(mq_id,IPC_RMID,NULL);
        sem_destroy(sstats);
        shmdt(sstats);
        sem_destroy(slog);
        shmdt(slog);
        shmdt(stats);
        close(pipe_fd);
        msync(log_map,LOG_SIZE,MS_SYNC);
        munmap(log_map,LOG_SIZE);
        close(log_fd);
        unlink(PIPE);
        printf("Tudo eliminado! A sair...\n");              //MQ, pipe, MMF, etc...
        exit(0);
    }
    else if (signum == SIGUSR1){
    	cria_estatisticas(stats);
        printf("Pacientes triados: %d\n",stats->triaged_patients);
        printf("Pacientes atendidos: %d\n",stats->attended_patients);
        printf("Tempo médio de espera para triagem: %.2f\n",stats->mean_triage_wait);
        printf("Tempo médio de espera pelo atendimento: %.2f\n",stats->mean_attendance_wait);
        printf("Tempo medio gasto no sistema: %.2f\n", stats->mean_total_time);
        printf("Tempo total de espera para triagem: %.2f\n",stats->total_triage_wait);
        printf("Tempo total de espera pelo atendimento: %.2f\n",stats->total_attendance_wait);
        printf("Tempo total gasto no sistema: %.2f\n", (stats->total_triage_wait+stats->total_attendance_wait));
    }
}


Patient getpatient(char str[STR_SIZE]){
    Patient pat;
    pat.name = strtok(str," ");
    pat.triage_time = strtof(strtok(NULL," "),NULL);
    pat.attendance_time = strtof(strtok(NULL," "),NULL);
    pat.priority = strtol(strtok(NULL," "),NULL,10);
    pat.arrival_time = time(NULL);
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


void* triage(void *p){
	char str[STR_SIZE];
    Thread *data= (Thread *) p;
    sprintf(str,"Thread %d is starting!\n",data->thread_number);
    strncat(log_map,str,strlen(str));
    Patient pat;
    Message msg;
    pthread_mutex_lock(&queue_mutex);
    if (!empty_queue(queue)) {
        pat = dequeue(data->queue);
        pthread_mutex_unlock(&queue_mutex);
        msg.patient = pat;
        msg.m_type = pat.priority;
        if (msgsnd(mq_id, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
            perror("Nao foi possivel enviar mensagem");
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&stats_mutex);
        stats->triaged_patients++;
        stats->total_triage_wait += msg.patient.triage_time;
        sprintf(str,"Pacientes triados: %d\n", data->stats->triaged_patients);
        strncat(log_map,str,strlen(str));
        pthread_mutex_unlock(&stats_mutex);
        usleep(msg.patient.triage_time*1000);
    } else {
        pthread_mutex_unlock(&queue_mutex);
        //printf("Nao ha pacientes para enviar mensagem\n");
    }
    sprintf(str,"Thread %d a sair! Pacientes triados: %d\n",data->thread_number,data->stats->triaged_patients);
    strncat(log_map,str,strlen(str));
    pthread_exit(NULL);
}

void *piperead(void *p){
    char str[STR_SIZE];
    Patient pat;
    int group;
    int i;
    int g = 0;
    Queue *q = (Queue *) p;
    while(1){
        if (read(pipe_fd,str,sizeof(str)) > 0)
            pat = getpatient(str);
        if ((group = strtol(pat.name,NULL,10)) > 0){
            g++;
            for (i = 1; i <= group; i++){
                sprintf(pat.name,"Grupo %d - Paciente %d",g,i);
                printf("%s\n", pat.name);
                enqueue(q,pat);
                printf("Paciente %s lido e inserido na queue\n",pat.name);
            }
        } else if (!strcmp(pat.name,TRIAGE)) {
            num_triage = pat.priority;
            printf("Numero de triagens lido: %d\n",num_triage);
        } else {
            enqueue(q,pat);
            printf("Paciente %s lido e inserido na queue\n",pat.name);
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

void cria_estatisticas(Stats *stats){
	stats->mean_attendance_wait = (stats->total_attendance_wait)/stats->attended_patients;
	stats->mean_triage_wait = (stats->total_triage_wait)/stats->triaged_patients;
	stats->mean_total_time = (stats->total_triage_wait+stats->total_attendance_wait)/stats->attended_patients;
}

void temp_doctor() {
	char str[STR_SIZE];
    sprintf(str,"Doutor temporario a entrar\n");
    sem_wait(slog);
    strncat(log_map,str,strlen(str));
    sem_post(slog);
    Message msg;
    sem_wait(sstats);
    while(stats->triaged_patients - stats->attended_patients > 0.8 * mq_max){
        sem_post(sstats);
        msgrcv(mq_id,&msg,sizeof(msg) - sizeof(long),-10,0);
        sem_wait(sstats);
    	stats->attended_patients = stats->attended_patients + 1;
        stats->mean_attendance_wait += msg.patient.attendance_time;
        sem_post(sstats);
        usleep(msg.patient.attendance_time * 1000);
    }
    sem_wait(sstats);
    sprintf(str,"Doutor temporario a sair. Pacientes atendidos: %d \n", stats->attended_patients);
    sem_post(sstats);
    sem_wait(slog);
    strncat(log_map,str,strlen(str));
    sem_post(slog);
}


void doctor(){
    time_t time1 = time(NULL);
    char str[STR_SIZE];
    sprintf(str,"Doutor %d a iniciar o turno.\n",getpid());
    sem_wait(slog);
    strncat(log_map,str,strlen(str));
    sem_post(slog);
    Message msg;
    while(time(NULL)-time1 < shift_length) {
        if (msgrcv(mq_id,&msg,sizeof(msg) - sizeof(long),-10,IPC_NOWAIT) == -1 && errno != ENOMSG) {
            perror("Nao deu para ler da message queue\n");
            exit(0);
        }
        else if(errno == ENOMSG) {
            ;
        }
        else if(errno == EAGAIN){
        	perror("Fila cheia e IPC_NOWAIT selecionado");
        }
        else{
        	sprintf(str,"Retirou o paciente %s da queue\n",msg.patient.name);
            sem_wait(slog);
        	strncat(log_map,str,strlen(str));
            sem_post(slog);
        	sprintf(str,"Atendeu o paciente %s durante %f\n",msg.patient.name,msg.patient.attendance_time);
            sem_wait(slog);
        	strncat(log_map,str,strlen(str));
            sem_post(slog);
            sem_wait(sstats);
	        stats->attended_patients = stats->attended_patients + 1;
	        stats->total_attendance_wait = stats->total_attendance_wait + msg.patient.attendance_time;
            sem_post(sstats);
	        usleep(msg.patient.attendance_time*1000);
        }
        
    }
    sem_wait(sstats);
    sprintf(str,"Doutor %d a terminar o turno.\nPacientes atendidos: %d \n",getpid(),stats->attended_patients);
    sem_post(sstats);
    sem_wait(slog);
    strncat(log_map,str,strlen(str));
    sem_post(slog);
}