//
// Created by guilherme on 5-11-2017.
//

#ifndef PROJETOSO_STRUCTS_H
#define PROJETOSO_STRUCTS_H

#define LOGSIZE 4096

//Variaveis Globais
int num_doctors, num_triage, mq_max, shift_length = 0;

typedef struct {
	int triaged_patients;
	int attended_patients;
	float mean_triage_wait;
	float mean_attendance_time;
	float mean_total_time;
} Stats;

//Paciente
typedef struct patient{
    int arrival_number;
    char name[50];
    int triage_time;
    int priority;
    int attendance_time;
} Patient;

typedef struct {
    long mtype;
    Patient patient;
} Message;

//Fila de Pacientes
typedef struct queue * Queue;

typedef struct queue{
    Patient* patient;
    Queue next;
} Queue_node;

//Dados de uma Thread
typedef struct thread_data{
    Queue *queue;
    int thread_number;
    Stats *stats;
} Thread;

void doctor(Stats *);

#endif //PROJETOSO_STRUCTS_H