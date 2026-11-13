#include "Hmm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_COUNT 100
#define BLOCK_SIZE 1024

static void check_data(unsigned char *ptr, unsigned char value){
    for(size_t i = 0; i < BLOCK_SIZE; i++)
        assert(ptr[i] == value);
}

static void test_pattern(int pattern){
    unsigned char *ptr[BLOCK_COUNT];
    bool freed[BLOCK_COUNT] = {false};
    unsigned char *initial_break = program_break;
    unsigned char *old_break = program_break;
    size_t growth_count = 0;

    for(int i = 0; i < BLOCK_COUNT; i++){
        ptr[i] = HmmAlloc(BLOCK_SIZE);
        assert(ptr[i] != NULL);
        memset(ptr[i], i + 1, BLOCK_SIZE);

        if(program_break != old_break)
            growth_count++;
        old_break = program_break;
    }

    unsigned char *break_after_alloc = program_break;
    assert(break_after_alloc > initial_break);
    assert(growth_count < BLOCK_COUNT);

    for(int i = 0; i < BLOCK_COUNT; i++){
        bool should_free = false;

        if(pattern == 1)
            should_free = (i % 2 == 0);
        else if(pattern == 2)
            should_free = (i < BLOCK_COUNT - 1);
        else if(pattern == 3)
            should_free = (i >= BLOCK_COUNT / 2);

        if(should_free){
            HmmFree(ptr[i]);
            freed[i] = true;
        }
    }

    unsigned char *break_after_free = program_break;

    if(pattern == 3)
        assert(break_after_free < break_after_alloc);
    else
        assert(break_after_free == break_after_alloc);

    for(int i = 0; i < BLOCK_COUNT; i++){
        if(!freed[i])
            check_data(ptr[i], i + 1);
    }

    for(int i = 0; i < BLOCK_COUNT; i++){
        if(!freed[i])
            HmmFree(ptr[i]);
    }

    printf("Pattern %d: PASS | break: before=%td, after=%td, growth changes=%zu\n",
           pattern, break_after_alloc - initial_break,
           break_after_free - initial_break, growth_count);
}

int main(void){
    test_pattern(1);
    test_pattern(2);
    test_pattern(3);
    puts("PASS: simulated program-break patterns and live data integrity");
    return 0;
}
