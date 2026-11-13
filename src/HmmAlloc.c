/*
    File : HmmAlloc.c
    Desc : Heap Memory Manager allocation function stubs
*/

#include "Hmm.h"
#include <stdint.h>

#define HEAP_CAPACITY (1024 * 1024)

static _Alignas(max_align_t) unsigned char heap_area[HEAP_CAPACITY];
unsigned char *program_break = heap_area;
static unsigned char *heap_limit = heap_area + HEAP_CAPACITY;


block *free_head = NULL;
block *last_physical = NULL;

void insert_at_beginning_free(block *new_block){
	if(new_block == NULL)
		return;
	new_block->previous_free = NULL;
	new_block->next_free = free_head;
	if(free_head != NULL)
		free_head->previous_free =new_block;
	free_head = new_block;
}
void remove_free_block(block *target){
    if(target == NULL)
        return;

    if(target->previous_free != NULL)
        target->previous_free->next_free = target->next_free;
    else
        free_head = target->next_free;

    if(target->next_free != NULL)
        target->next_free->previous_free = target->previous_free;

    target->previous_free = NULL;
    target->next_free = NULL;
}
static block *find_first_fit(size_t size){
    block *current = free_head;

    while(current != NULL){
        if(current->size >= size)
            return current;

        current = current->next_free;
    }

    return NULL;
}
static size_t align_size(size_t size){
    if(size > SIZE_MAX - (ALIGNMENT - 1))
        return 0;

    return ((size + ALIGNMENT - 1) / ALIGNMENT) * ALIGNMENT;
}

static unsigned char *increase_program_break(size_t size){
    if(size == 0)
        return NULL;

    size_t available = heap_limit - program_break;

    if(size > available)
        return NULL;

    unsigned char *old_break = program_break;

    program_break += size;

    return old_break;
}

static block *grow_heap(size_t size){
    size_t required;

    if(last_physical != NULL && last_physical->is_free){
        if(last_physical->size >= size)
            return last_physical;

        required = size - last_physical->size;
    }
    else{
        if(size > HEAP_CAPACITY - HEADER_SIZE)
            return NULL;

        required = size + HEADER_SIZE;
    }

    size_t chunks = required / HEAP_CHUNK_SIZE;

    if(required % HEAP_CHUNK_SIZE != 0)
        chunks++;

    if(chunks > (size_t)(heap_limit - program_break) / HEAP_CHUNK_SIZE)
        return NULL;

    size_t grow_size = chunks * HEAP_CHUNK_SIZE;

    unsigned char *start = increase_program_break(grow_size);

    if(start == NULL)
        return NULL;

    if(last_physical != NULL && last_physical->is_free){
        last_physical->size += grow_size;
        return last_physical;
    }

    block *new_block = (block *)start;

    new_block->size = grow_size - HEADER_SIZE;
    new_block->is_free = true;

    new_block->previous_physical = last_physical;
    new_block->previous_free = NULL;
    new_block->next_free = NULL;

    last_physical = new_block;

    insert_at_beginning_free(new_block);

    return new_block;
}

static void split_block(block *current, size_t size){
    if(current->size - size < HEADER_SIZE + ALIGNMENT)
        return;

    unsigned char *next_address = (unsigned char *)current + HEADER_SIZE + current->size;
    block *remaining = (block *)((unsigned char *)current + HEADER_SIZE + size);

    remaining->size = current->size - size - HEADER_SIZE;
    remaining->is_free = true;
    remaining->previous_physical = current;
    remaining->previous_free = NULL;
    remaining->next_free = NULL;

    if(next_address < program_break){
        block *next = (block *)next_address;
        next->previous_physical = remaining;
    }
    else
        last_physical = remaining;

    current->size = size;
    insert_at_beginning_free(remaining);
}

void *HmmAlloc(size_t size){
    if(size == 0)
        return NULL;

    size_t aligned_size = align_size(size);
    if(aligned_size == 0)
        return NULL;

    block *current = find_first_fit(aligned_size);

    if(current == NULL)
        current = grow_heap(aligned_size);

    if(current == NULL)
        return NULL;

    remove_free_block(current);
    split_block(current, aligned_size);
    current->is_free = false;

    return (unsigned char *)current + HEADER_SIZE;
}

void *hmmCalloc(size_t nmemb, size_t size)
{
  (void)nmemb;
  (void)size;
  return NULL;
}

void *hmmReAlloc(void *ptr, size_t reqSize)
{
  (void)ptr;
  (void)reqSize;
  return NULL;
}

size_t hmmSize(void *ptr)
{
  (void)ptr;
  return 0U;
}
