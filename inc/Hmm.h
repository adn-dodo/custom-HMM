/*
    File : Hmm.h
    Desc : Heap Memory Manager include file
*/

#ifndef HMM_H
#define HMM_H

#include <stddef.h>
#include <stdbool.h>
typedef struct block {

    size_t size;
    bool is_free;

    struct block *previous_physical;

    struct block *previous_free;
    struct block *next_free;

} block;

#define HEAP_CHUNK_SIZE 4096
#define ALIGNMENT _Alignof(max_align_t)
#define HEADER_SIZE (((sizeof(block) + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT)

extern block *free_head;
extern block *last_physical;
extern unsigned char *program_break;

void insert_at_beginning_free(block *new_block);
void remove_free_block(block *target);

void HmmFree(void *ptr);

void *HmmAlloc(size_t size);

void *hmmCalloc(size_t nmemb, size_t size);

void *hmmReAlloc(void *ptr, size_t reqSize);

size_t hmmSize(void *ptr);

#endif /* HMM_H */
