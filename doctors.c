//
// Created by guilherme on 26-10-2017.
//

#include "doctors.h"

void doctor(){
    time_t time1 = time(NULL);
    time_t time2,time3;
    printf("I'm doctor %d, and I will begin my shift.\n",getpid());
    do{
	time2 = time(NULL);
        time3 = difftime(time2,time1);
    } while(time3<shift_length);
    printf("I'm doctor %d, and I will end my shift.\n",getpid());
}