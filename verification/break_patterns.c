#define _DEFAULT_SOURCE
#include "Hmm.h"
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_COUNT 100
#define BLOCK_SIZE 1024

static void check_data(unsigned char *ptr, unsigned char value){
    for(size_t i = 0; i < BLOCK_SIZE; i++)
        assert(ptr[i] == value);
}

typedef struct {
    ptrdiff_t before;
    ptrdiff_t after;
    size_t growth_count;
} pattern_result;

static pattern_result test_pattern(int pattern){
    unsigned char *ptr[BLOCK_COUNT];
    bool freed[BLOCK_COUNT] = {false};
    unsigned char *initial_break = sbrk(0);
    assert(initial_break != (void *)-1);
    unsigned char *old_break = initial_break;
    size_t growth_count = 0;

    for(int i = 0; i < BLOCK_COUNT; i++){
        ptr[i] = HmmAlloc(BLOCK_SIZE);
        assert(ptr[i] != NULL);
        memset(ptr[i], i + 1, BLOCK_SIZE);

        unsigned char *new_break = sbrk(0);
        assert(new_break != (void *)-1);
        if(new_break != old_break)
            growth_count++;
        old_break = new_break;
    }

    unsigned char *break_after_alloc = sbrk(0);
    assert(break_after_alloc != (void *)-1);
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

    unsigned char *break_after_free = sbrk(0);
    assert(break_after_free != (void *)-1);

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

    assert(sbrk(0) == initial_break); // Full free must restore the original break.

    pattern_result result = {
        .before = break_after_alloc - initial_break,
        .after = break_after_free - initial_break,
        .growth_count = growth_count
    };
    return result;
}

int main(void){
    // No stdio until the tests finish: libc's malloc can also change the break.
    pattern_result results[3];
    for(int i = 0; i < 3; i++)
        results[i] = test_pattern(i + 1);

    for(int i = 0; i < 3; i++){
        printf("Pattern %d: PASS | real break: before=%td, after=%td, growth changes=%zu\n",
               i + 1, results[i].before, results[i].after, results[i].growth_count);
    }
    puts("PASS: real sbrk program-break patterns and live data integrity");
    return 0;
}
