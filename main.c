#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <time.h>

#include "threadpool.h"

#define DJB2_INIT 5381
#define NUM_OF_THREADS  3
#define BLOOM_SIZE 30

double djb2_time;
double sdbm_time;
double total;

Pool* pool;

typedef struct BloomFilter
{
    int bits[BLOOM_SIZE];
    int size;
    pthread_mutex_t mutexes[BLOOM_SIZE];
    
} BloomFilter;

struct put_params
{
    char* string;
    int length;
} put_params;


BloomFilter* bloomFilter;

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

BloomFilter* createBloomFilter(size_t size)
{
    BloomFilter* bloomFilter = malloc(sizeof(BloomFilter));

    bloomFilter->size = size;

    for(int i=0;i<BLOOM_SIZE;i++)
    {
        pthread_mutex_init(&(bloomFilter->mutexes[i]), NULL);
    }

    return bloomFilter;
}

bool getBloomFilterValue(size_t position)
{
    if (position >= bloomFilter->size)
    {
        printf("position is out of bounds for the bit vector");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&(bloomFilter->mutexes[position]));

    bool bit = bloomFilter->bits[position] == 1;

    pthread_mutex_unlock(&(bloomFilter->mutexes[position]));

    return bit; 
}

void setBloomFilterValue(size_t position, bool value)
{
    if (position >= bloomFilter->size)
    {
        printf("position is out of bounds for the bit vector");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_lock(&(bloomFilter->mutexes[position]));

    bloomFilter->bits[position] = value;

    pthread_mutex_unlock(&(bloomFilter->mutexes[position]));
}

void putValue(void* args)
{

    struct put_params *wub = (struct put_params *) args;
    char* data = wub->string;
    int length = wub->length;

    uint32_t hash = djb2(data, length) % bloomFilter->size;
    //printf("The djb2 hash value is %d\n", hash);
    setBloomFilterValue(hash, 1);

    hash = sdbm(data, length) % bloomFilter->size;
    //printf("The sdbm hash value is %d\n", hash);
    setBloomFilterValue(hash, 1);

}

void testValue(void* data, size_t length)
{
    bool bit = true;

    uint32_t hash = djb2(data, length) % bloomFilter->size;
    //printf("The djb2 hash value is %d\n", hash);
    bit &= getBloomFilterValue(hash);

    //printf("That bit is: %d\n", bit);

    hash = sdbm(data, length) % bloomFilter->size;
    //printf("The sdbm hash value is %d\n", hash);
    bit &= getBloomFilterValue(hash);
 
    //printf("\n\nResult: %d\n", bit);
}

void printBloomFilter()
{
    printf("\n\n---BloomFilter---\n");
    for (int i = 0; i < bloomFilter->size; i++)
    {
        printf("%d", getBloomFilterValue(i));
    }
}



// https://stackoverflow.com/questions/49595265/how-to-split-a-text-file-using-tab-space-in-c
void split_string(char *line) {
    
    struct put_params *params;
    
    const char delimiter[] = "\t";
    char *tmp;

    tmp = strtok(line, delimiter);
    if (tmp == NULL)
    {
        return;
    }

    //printf("%s\n", tmp);
    //putValue(BloomFilter, tmp, strlen(tmp));
    
    params = calloc(1, sizeof(struct put_params));

    params->string = tmp;
    params->length = strlen(tmp);

    addWork(pool, (threadFunction) &putValue, params);

    for (;;) {
        tmp = strtok(NULL, delimiter);
        
        if (tmp == NULL)
            break;
        // printf("%s\n", tmp);
        // putValue(BloomFilter, tmp, strlen(tmp));

        params = calloc(1, sizeof(struct put_params));

        params->string = tmp;
        params->length = strlen(tmp);

        addWork(pool, (threadFunction) &putValue, params);
    }
}

void readFile(char* fileName)
{  
    char *line = NULL;
    size_t size;
    FILE *fp = fopen(fileName, "r");

    if (fp == NULL) {
        return;
    }

    while (getline(&line, &size, fp) != -1) {
        split_string(line);
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

    for (int i = 0; i < 23002; i++)
    {
        //addWork(pool, bla, NULL);
        //sleep(1);
    }

    clock_t begin = clock();

    bloomFilter = createBloomFilter(BLOOM_SIZE);
    readFile("dataset-string-matching_train.txt");
  
    // printf("Adfadf");
    // printf("size of the vector is : %d\n", BloomFilter->size);
    // printf("Printin BloomFilter:\n\n");

    // srand(2032);

    // for (int i = 0; i < BloomFilter->size; i++)
    // {
    //     int value = rand() % 2;
    //     printf("setting %d to %d\n", i, value);
    //     setBloomFilterValue(BloomFilter, i, value);
    // }

    // char* s = "asdflkassdf";
    // char* t = "sasdflkasdf1";

    // //addWork(pool, putValue, )
    // putValue(BloomFilter, s, strlen(s));
    // testValue(BloomFilter, s, strlen(s));

    // testValue(BloomFilter, t, strlen(t));
    
    // putValue(BloomFilter, t, strlen(t)); 
    // testValue(BloomFilter, t, strlen(t));     

    //printBloomFilter(BloomFilter);

    clock_t end = clock();

    total = end - begin;

    printf("\n\n");
    printf("djb2 time: %f\n", djb2_time);
    printf("sdbm time: %f\n", sdbm_time);
    printf("total time: %f\n", total);

}