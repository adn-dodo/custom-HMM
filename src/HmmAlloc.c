/*
    File : HmmAlloc.c
    Desc : Heap Memory Manager allocation functions
*/

#define _DEFAULT_SOURCE
#include "Hmm.h"
#include <stdint.h>
#include <unistd.h>

unsigned char *program_break = NULL;
unsigned char *initial_program_break = NULL;


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
    if(size == 0 || size > (size_t)INTPTR_MAX)
        return NULL;

    void *current_break = sbrk(0);
    if(current_break == (void *)-1)
        return NULL;

    // Never extend our blocks over memory another allocator added.
    if(program_break != NULL && current_break != program_break)
        return NULL;

    size_t padding = 0;
    if(program_break == NULL){
        padding = (ALIGNMENT - ((uintptr_t)current_break % ALIGNMENT)) % ALIGNMENT;
        if(size > (size_t)INTPTR_MAX - padding)
            return NULL;
    }

    void *old_break = sbrk((intptr_t)(size + padding));
    if(old_break == (void *)-1)
        return NULL;

    unsigned char *start = (unsigned char *)old_break + padding;

    if(program_break == NULL)
        initial_program_break = (unsigned char *)old_break;

    program_break = start + size;
    return start;
}

static block *grow_heap(size_t size){
    size_t required;

    if(last_physical != NULL && last_physical->is_free){
        if(last_physical->size >= size)
            return last_physical;

        required = size - last_physical->size;
    }
    else{
        if(size > (size_t)INTPTR_MAX - HEADER_SIZE)
            return NULL;

        required = size + HEADER_SIZE;
    }

    size_t chunks = required / HEAP_CHUNK_SIZE;
    if(required % HEAP_CHUNK_SIZE != 0)
        chunks++;

    if(chunks > (size_t)INTPTR_MAX / HEAP_CHUNK_SIZE)
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
