/* 
    File : heapWrappers.c
    Desc : Wrappers for all the needed functions in the heap functions family
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Hmm.h"

void * malloc(size_t size)
{
    char buffer[32];
    size_t n = size;
    int i = sizeof(buffer);

    buffer[--i] = '\n';

    do {
        buffer[--i] = '0' + (n % 10);
        n /= 10;
    } while(n != 0);

    write(2, "malloc requested: ", 18);
    write(2, buffer + i, sizeof(buffer) - i);

    return HmmAlloc(size);
}

void free(void *ptr)
{
    HmmFree(ptr);
}

void *calloc(size_t nmemb, size_t size)
{
	return hmmCalloc(nmemb, size);
}

void* realloc(void *ptr, size_t size)
{
	return hmmReAlloc(ptr, size);
}
