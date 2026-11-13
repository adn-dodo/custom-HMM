/*
    File : HmmFree.c
    Desc : Heap Memory Manager free function
*/

#include "Hmm.h"

#define TRIM_THRESHOLD (2 * HEAP_CHUNK_SIZE)

static block *get_next_physical(block *current){
    unsigned char *next_address = (unsigned char *)current + HEADER_SIZE + current->size;

    if(next_address >= program_break)
        return NULL;

    return (block *)next_address;
}

static void trim_heap(void){
    if(last_physical == NULL || !last_physical->is_free)
        return;

    size_t total = HEADER_SIZE + last_physical->size;

    if(total <= HEAP_CHUNK_SIZE)
        return;

    size_t released = ((total - HEAP_CHUNK_SIZE) / HEAP_CHUNK_SIZE) * HEAP_CHUNK_SIZE;

    if(released < TRIM_THRESHOLD)
        return;

    program_break -= released;
    last_physical->size -= released;
}

void HmmFree(void *ptr){
    if(ptr == NULL)
        return;

    block *current = (block *)((unsigned char *)ptr - HEADER_SIZE);

    if(current->is_free)
        return;

    current->is_free = true;

    block *next = get_next_physical(current);

    if(next != NULL && next->is_free){
        remove_free_block(next);
        current->size += HEADER_SIZE + next->size;

        block *after = get_next_physical(current);

        if(after != NULL)
            after->previous_physical = current;
        else
            last_physical = current;
    }

    block *previous = current->previous_physical;

    if(previous != NULL && previous->is_free){
        remove_free_block(previous);
        previous->size += HEADER_SIZE + current->size;
        current = previous;

        block *after = get_next_physical(current);

        if(after != NULL)
            after->previous_physical = current;
        else
            last_physical = current;
    }

    insert_at_beginning_free(current);
    trim_heap();
}
