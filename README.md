# Custom Heap Memory Manager (HMM)

This is my implementation of a Heap Memory Manager in C, starting from the original project skeleton.

I used real `sbrk()` to manage the heap and implemented the allocation and free functions, with First Fit, splitting, coalescing, and heap trimming.

## How it works

- **First Fit:** I search the free list and use the first block large enough for the requested size.
- **Heap growth:** If no suitable block is available, I increase the program break in 4096-byte chunks. If the last physical block is free, I extend it instead of creating another block.
- **Splitting:** If a free block is larger than needed, I split it and keep the remaining part in the free list.
- **Coalescing:** When a block is freed, I merge it with adjacent free blocks when possible.
- **Trimming:** I use negative `sbrk()` to return unused memory at the end of the heap when possible.
- **Alignment:** Allocated blocks are aligned using `max_align_t`.

The free blocks are kept in a doubly linked free list. I also track the current program break to avoid growing my blocks over memory added by something else.

## Functions

| Function | Purpose |
| --- | --- |
| `HmmAlloc()` | Allocate memory |
| `HmmFree()` | Free memory and coalesce neighboring free blocks |
| `hmmCalloc()` | Allocate and initialize memory to zero |
| `hmmReAlloc()` | Reallocate memory while preserving existing data |
| `hmmSize()` | Return the usable size of an allocated block |

The original wrappers in `src/heapWrappers.c` connect these functions to `malloc()`, `free()`, `calloc()`, and `realloc()`.

## Build and test

On Linux with GCC and Make installed:

```bash
make
make shared_lib
```

This builds the static library, shared library, and test program.

Run the tests:

```bash
./build/hmm_test
make verify
```

All five tests in `test/main.c` and the three program-break verification patterns passed in my testing.

To remove build files:

```bash
make clean
```

## Using LD_PRELOAD

I also tested the shared library with Linux commands:

```bash
LD_PRELOAD="$(pwd)/build/lib/libhmm.so" /usr/bin/ls -la
```

I tested `ls`, `grep`, and `wc` successfully.

This is a learning project, not a complete replacement for glibc's allocator. It is not thread-safe, and it does not implement allocation interfaces such as `posix_memalign()`. Some programs, including the Rust version of `sort` on my system, can fail because they use other allocation paths.

## Flowcharts

The flowcharts for `HmmAlloc()` and `HmmFree()` are in [FLOWCHARTS.md](FLOWCHARTS.md).
