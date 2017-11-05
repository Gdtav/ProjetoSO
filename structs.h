//
// Created by guilherme on 5-11-2017.
//

#ifndef PROJETOSO_STRUCTS_H
#define PROJETOSO_STRUCTS_H

//Variaveis Globais
int num_doctors, num_triage, mq_max, triaged_pacients, shift_length = 0;;

//Paciente
typedef struct pacient{
    int arrival_number;
    char name[50];
    int triage_time;
    int priority;
    int attendance_time;
} Pacient;

//Fila de Pacientes
typedef struct queue * Queue;

typedef struct queue{
    Pacient* pacient;
    Queue next;
} Queue_node;

//Dados de uma Thread
typedef struct thread_data{
    Queue *queue;
    int thread_number;
} Thread;

void doctor();

#endif //PROJETOSO_STRUCTS_H