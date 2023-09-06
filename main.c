#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#include <time.h>

#include "threadpool.h"

#define DJB2_INIT 5381
#define NUM_OF_THREADS  3
#define BLOOM_SIZE 30

double djb2_time;
double sdbm_time;
double total;

Pool* pool;

typedef struct bitVector
{
    int bits[BLOOM_SIZE];
    int size;
    pthread_mutex_t mutexes[BLOOM_SIZE];
    
} bitVector;


/*

hash functions

*/

uint32_t djb2(void *buff, size_t length) {

    clock_t begin = clock();

    uint32_t hash = DJB2_INIT;
    const uint8_t *data = buff;
    for(size_t i = 0; i < length; i++) {
         hash = ((hash << 5) + hash) + data[i]; 
    }

    clock_t end = clock();

    djb2_time += end - begin;

    return hash;
}
uint32_t sdbm(void *buff, size_t length) {

clock_t begin = clock();

    uint32_t hash = 0;
    const uint8_t *data = buff;
    for(size_t i = 0; i < length; i++) {
        hash = data[i] + (hash << 6) + (hash << 16) - hash;
    }

    clock_t end = clock();

    sdbm_time += end - begin;

    return hash;
}

bitVector* createBitVector(size_t size)
{
    bitVector* vector = malloc(sizeof(bitVector));

    vector->size = size;

    for(int i=0;i<BLOOM_SIZE;i++)
    {
        pthread_mutex_init(&(vector->mutexes[i]), NULL);
    }

    return vector;
}

bool getBitVectorValue(bitVector* bitVector, size_t position)
{
    if (position >= bitVector->size)
    {
        printf("position is out of bounds for the bit vector");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&(bitVector->mutexes[position]));

    bool bit = bitVector->bits[position] == 1;

    pthread_mutex_unlock(&(bitVector->mutexes[position]));

    return bit; 
}

void setBitVectorValue(bitVector* bitVector, size_t position, bool value)
{
    if (position >= bitVector->size)
    {
        printf("position is out of bounds for the bit vector");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&(bitVector->mutexes[position]));

    bitVector->bits[position] = value;

    pthread_mutex_unlock(&(bitVector->mutexes[position]));
}

void putValue(bitVector* bitVector, void* data, int length)
{

    uint32_t hash = djb2(data, length) % bitVector->size;
    //printf("The djb2 hash value is %d\n", hash);
    setBitVectorValue(bitVector, hash, 1);

    hash = sdbm(data, length) % bitVector->size;
    //printf("The sdbm hash value is %d\n", hash);
    setBitVectorValue(bitVector, hash, 1);

}

void testValue(bitVector* bitVector, void* data, size_t length)
{
    bool bit = true;

    uint32_t hash = djb2(data, length) % bitVector->size;
    //printf("The djb2 hash value is %d\n", hash);
    bit &= getBitVectorValue(bitVector, hash);

    //printf("That bit is: %d\n", bit);

    hash = sdbm(data, length) % bitVector->size;
    //printf("The sdbm hash value is %d\n", hash);
    bit &= getBitVectorValue(bitVector, hash);
 
    //printf("\n\nResult: %d\n", bit);
}

void printBitVector(bitVector* bitVector)
{
    printf("\n\n---BitVector---\n");
    for (int i = 0; i < bitVector->size; i++)
    {
        printf("%d", getBitVectorValue(bitVector, i));
    }
}

struct put_params
{
    bitVector* bitVector;
    char* string;
    int length;
} put_params;

// https://stackoverflow.com/questions/49595265/how-to-split-a-text-file-using-tab-space-in-c
void split_string(bitVector* bitVector, char *line) {
    
    struct put_params *params = malloc(sizeof(struct put_params));
    
    const char delimiter[] = "\t";
    char *tmp;

    tmp = strtok(line, delimiter);
    if (tmp == NULL)
    return;

    //printf("%s\n", tmp);
    //putValue(bitVector, tmp, strlen(tmp));
    
    params->bitVector = bitVector;
    params->string = tmp;
    params->length = strlen(tmp);

    addWork(pool, putValue, params);

    for (;;) {
        tmp = strtok(NULL, delimiter);
        if (tmp == NULL)
            break;
        //printf("%s\n", tmp);
        // putValue(bitVector, tmp, strlen(tmp));

        params->bitVector = bitVector;
        params->string = tmp;
        params->length = strlen(tmp);

        addWork(pool, putValue, params);
    }
}

void readFile(bitVector* bitVector, char* fileName)
{  
    char *line = NULL;
    size_t size;
    FILE *fp = fopen(fileName, "r");

    if (fp == NULL) {
        return;
    }

    while (getline(&line, &size, fp) != -1) {
        split_string(bitVector, line);
    }

    free(line);
}

void bla()
{
    printf("blabalb!");
}

int main () 
{

    pool = createThreadPool(4);

    // for (int i = 0; i < 23002; i++)
    // {
    //     addWork(pool, bla, NULL);
    // }

    clock_t begin = clock();

    bitVector* bitVector = createBitVector(BLOOM_SIZE);
    readFile(bitVector, "dataset-string-matching_train.txt");
  
    // printf("Adfadf");
    // printf("size of the vector is : %d\n", bitVector->size);
    // printf("Printin bitvector:\n\n");

    // srand(2032);

    // for (int i = 0; i < bitVector->size; i++)
    // {
    //     int value = rand() % 2;
    //     printf("setting %d to %d\n", i, value);
    //     setBitVectorValue(bitVector, i, value);
    // }

    // char* s = "asdflkassdf";
    // char* t = "sasdflkasdf1";

    // //addWork(pool, putValue, )
    // putValue(bitVector, s, strlen(s));
    // testValue(bitVector, s, strlen(s));

    // testValue(bitVector, t, strlen(t));
    
    // putValue(bitVector, t, strlen(t)); 
    // testValue(bitVector, t, strlen(t));     

    //printBitVector(bitVector);

    clock_t end = clock();

    total = end - begin;

    printf("\n\n");
    printf("djb2 time: %f\n", djb2_time);
    printf("sdbm time: %f\n", sdbm_time);
    printf("total time: %f\n", total);

}