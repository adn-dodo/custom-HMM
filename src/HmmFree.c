/*
    File : HmmFree.c
    Desc : Heap Memory Manager free function
*/

#define _DEFAULT_SOURCE
#include "Hmm.h"
#include <stdint.h>
#include <unistd.h>

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

    void *current_break = sbrk(0);
    if(current_break == (void *)-1 || current_break != program_break)
        return;

    // Everything is free: return the entire arena, including alignment padding.
    if(last_physical->previous_physical == NULL){
        size_t released = (uintptr_t)program_break - (uintptr_t)initial_program_break;
        if(released > (size_t)INTPTR_MAX)
            return;

        block *last = last_physical;
        remove_free_block(last);
        if(sbrk(-((intptr_t)released)) == (void *)-1){
            insert_at_beginning_free(last);
            return;
        }

        last_physical = NULL;
        program_break = NULL;
        initial_program_break = NULL;
        return;
    }

    // Return the entire free tail once it is large enough to justify sbrk().
    size_t released = HEADER_SIZE + last_physical->size;
    if(released < TRIM_THRESHOLD || released > (size_t)INTPTR_MAX)
        return;

    block *last = last_physical;
    block *previous = last->previous_physical;
    remove_free_block(last);

    if(sbrk(-((intptr_t)released)) == (void *)-1){
        insert_at_beginning_free(last);
        return;
    }

    last_physical = previous;
    program_break -= released;
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
