# AdnAlloc — Heap Memory Manager

AdnAlloc is a C11 heap allocator built inside the instructor-provided project layout. It uses a **simulated** program break: the backing storage is a fixed-size, suitably aligned byte array, rather than memory obtained with `sbrk()`.

## Current status

- Implemented: `HmmAlloc()`, `HmmFree()`, First Fit, free-list insertion/removal, block splitting, immediate coalescing, aligned allocations, chunk-based heap growth, and controlled heap shrinking.
- Still TODO: `hmmCalloc()`, `hmmReAlloc()`, and `hmmSize()` (currently placeholders). The complete instructor test therefore does **not** pass yet.
- The original directory structure, public function signatures, `Makefile`, and `heapWrappers.c` have been preserved. No extra `.c` or `.h` files have been added.

## Design

**Backing memory and break.** `heap_area` holds 1 MiB of storage. `program_break` initially points to its beginning; `heap_limit` marks the end of the array. Raising or lowering `program_break` only changes the *logically managed* region; it does not change the process's real program break. Growth occurs in 4096-byte chunks when no suitable free block can be found.

**Block layout.** Each block contains a header followed by a payload. The header stores usable payload capacity, allocation status, a pointer to the previous physical block, and two links used by the doubly linked free list. The next physical block can be calculated from the current header address, padded header size, and usable payload capacity. Both payloads and headers are aligned using `_Alignof(max_align_t)` and `HEADER_SIZE`.

**Allocation.** `HmmAlloc()` rejects zero-sized requests and checked-overflow alignment, then finds the first sufficiently large free block. If necessary, the simulated heap grows. It removes the selected block from the free list, splits it only if the remainder can hold a new aligned header and usable payload, marks it allocated, and returns its payload pointer.

**Freeing and coalescing.** `HmmFree(NULL)` does nothing. For a valid outstanding allocation, `HmmFree()` marks its block free, merges it with any physically adjacent free successor and predecessor, repairs the physical-neighbor links and free list, and inserts the resulting free block. No payload bytes need to be copied during coalescing.

**Controlled trimming.** When the last physical block is free, the allocator can decrease `program_break`. To avoid frequent changes, it trims only when at least **two complete 4096-byte chunks** can be released while retaining at least one chunk in the trailing free block. No still-allocated block is discarded. The backing array remains allocated throughout.

**Invariants.** Every free block is represented exactly once in the free list; allocated blocks are absent from it; there are no neighboring physically free blocks after a completed free; physical headers span the managed region without gaps; and `program_break` remains within the backing array.

## Limitations

- This is a learning allocator, not a production or thread-safe replacement for libc's allocator.
- `HmmFree(ptr)` requires a pointer returned by an active `HmmAlloc()` allocation. Arbitrary invalid pointers and double frees after coalescing are not guaranteed to be detected safely.
- The free list uses First Fit and may take O(n) time to find a block.
- The current fixed arena has a capacity of 1 MiB. Requests that cannot fit return `NULL`.
- Internal metadata and shared state remain exposed through `Hmm.h` to preserve the original project structure; they can be separated into an internal header in a later refactor.

## Build

```sh
make clean
make
```

This produces `build/lib/libhmm.a` and `build/hmm_test`. You can also run `make shared_lib` to build the shared library.

## Tests

The instructor's `test/main.c` exercises `malloc/free`, and also enables `calloc/realloc`. At the current stage `calloc/realloc` are not implemented, so **do not expect the complete test executable to pass**. The core allocation and free portions (with the two optional test sections disabled in a temporary copy) have passed. Independent tests also passed for memory reuse, alignment, coalescing, trimming, allocation failure, 20,000 randomized operations, and both AddressSanitizer and UndefinedBehaviorSanitizer.

The instructor's real-`sbrk(0)` check verifies that AdnAlloc does not change the OS program break, **not** that the simulated program break was reduced. The latter requires inspecting `program_break` or running targeted tests.

Do **not** use `LD_PRELOAD` with this project until all allocation-family functions and edge cases have been completed and extensively tested.

## Simulated program-break verification

The independent `verification/break_patterns.c` test adapts the **allocation/free patterns** discussed in *The Linux Programming Interface* `free_and_sbrk.c` example. It uses `HmmAlloc()`, `HmmFree()`, and AdnAlloc's exported `program_break` rather than `malloc()`, `free()`, and the OS's `sbrk(0)`.

The linked example uses much larger allocations; this test uses **100 blocks of 1024 bytes** to stay within the allocator's fixed 1 MiB backing arena.

- **Pattern 1:** Free every other block; the simulated break must not decrease while the final allocation remains active.
- **Pattern 2:** Free everything except the final block; the simulated break must not decrease.
- **Pattern 3:** Free the final 50 blocks; adjacent trailing free space should coalesce and the simulated break should decrease.

All tests also verify that the contents of still-allocated blocks remain unchanged and that the growth algorithm makes fewer break adjustments than individual allocation calls.

From the project root:

```sh
make

gcc -std=c11 -Wall -Wextra -g -Iinc \
    -o build/break_patterns \
    verification/break_patterns.c src/HmmAlloc.c src/HmmFree.c

./build/break_patterns
```

For sanitizer verification:

```sh
gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined \
    -fno-omit-frame-pointer -Iinc \
    -o build/break_patterns_san \
    verification/break_patterns.c src/HmmAlloc.c src/HmmFree.c

./build/break_patterns_san
```

**Observed results (November 2026):** All three patterns passed under normal compilation and with AddressSanitizer/UndefinedBehaviorSanitizer. In the third test, the managed heap was reduced from 106,496 bytes to 57,344 bytes. These results do not imply that the full instructor test (which also calls `calloc()` and `realloc()`) passes.

Reference: https://man7.org/tlpi/code/online/dist/memalloc/free_and_sbrk.c.html
