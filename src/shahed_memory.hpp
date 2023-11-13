#pragma once

#include <semaphore.h>
#include <string>

const char* shared_memory_name = "mySharedMemory";
const int prots = PROT_READ | PROT_WRITE;
const int flags = MAP_SHARED;
const int DataSize = 5;

struct SharedMemory {
    char data[DataSize];
    float value;
    int flag;
    sem_t semaphore1;
    sem_t semaphore2;
    sem_t semaphore3;
};