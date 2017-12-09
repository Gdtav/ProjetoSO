//
// Created by guilherme on 5-11-2017.
//

#ifndef PROJETOSO_STRUCTS_H
#define PROJETOSO_STRUCTS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/mman.h>

#define PIPE "input_pipe"
#define LOG "urgency.log"
#define TRIAGE "TRIAGE"
#define STR_SIZE 256
#define LOG_SIZE 1024*1024

//Variaveis Globais
int num_doctors, num_triage, mq_max, shift_length;
int mem_id, mq_id, sem_id;
int pipe_fd, log_fd;
sem_t *sem;


pthread_cond_t triage_threshold_cv = PTHREAD_COND_INITIALIZER;  //What's this for???
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

//Estatisticas 
typedef struct {
	int triaged_patients;
	int attended_patients;
	float mean_triage_wait;
	float mean_attendance_wait;
	float mean_total_time;
    float total_triage_wait;
    float total_attendance_wait;
    float total_time_spent;
} Stats;

Stats *stats;

//Paciente
typedef struct patient{
    int arrival_number;
    char name[50];
    time_t arrival_time;
    float triage_time;
    float attendance_time;
    int priority;
} Patient;

typedef struct {
    long m_type;
    Patient patient;
} Message;

//Fila de Pacientes
typedef struct node_type {
    Patient patient;
    struct node_type *next;
} Queue_node;

typedef Queue_node *q_ptr;

typedef struct {
    Queue_node *rear;
    Queue_node *front;
} Queue;

Queue *queue;

//Dados de uma Thread
typedef struct thread_data{
    Queue *queue;
    int thread_number;
    Stats *stats;
} Thread;

#endif //PROJETOSO_STRUCTS_H