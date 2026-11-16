# Heap Memory Manager Flowcharts

These flowcharts follow the implementation in `src/HmmAlloc.c` and `src/HmmFree.c`.

**Colors:** blue = entry, amber = decision, purple = operation, teal = heap / memory operation, green = normal exit, red = allocation failure. The diagrams remain editable Mermaid charts and render directly on GitHub.

## `HmmAlloc()` flowchart

```mermaid
flowchart TD
    A([HmmAlloc size]) --> B{Size is nonzero?}
    B -- No --> Z([Return NULL])
    B -- Yes --> C[Align requested size]
    C --> D{Alignment succeeded?}
    D -- No --> Z
    D -- Yes --> E[Find First Fit in free list]
    E --> F{Suitable free block found?}
    F -- Yes --> M[Remove block from free list]
    F -- No --> G{Last physical block is free?}
    G -- Yes --> H[Calculate extra space needed for that block]
    G -- No --> I[Calculate payload plus header space]
    H --> J[Round growth up to 4096-byte chunks]
    I --> J
    J --> K{Growth size is valid and sbrk succeeds?}
    K -- No --> Z
    K -- Yes --> L[Extend free tail or create a new free block]
    L --> M
    M --> N{Enough leftover space for a header and aligned payload?}
    N -- Yes --> O[Split block and insert free remainder]
    N -- No --> P[Keep entire block]
    O --> Q[Mark selected block allocated]
    P --> Q
    Q --> R([Return aligned payload pointer])

    %% Color palette
    classDef start fill:#DBEAFE,stroke:#2563EB,stroke-width:2px,color:#1E3A8A;
    classDef decision fill:#FEF3C7,stroke:#D97706,stroke-width:1.7px,color:#78350F;
    classDef process fill:#EDE9FE,stroke:#7C3AED,stroke-width:1.5px,color:#4C1D95;
    classDef memory fill:#CCFBF1,stroke:#0F766E,stroke-width:1.7px,color:#134E4A;
    classDef stop fill:#FEE2E2,stroke:#DC2626,stroke-width:2px,color:#7F1D1D;
    classDef success fill:#DCFCE7,stroke:#16A34A,stroke-width:2px,color:#166534;

    class A start;
    class B,D,F,G,K,N decision;
    class C,E,H,I,M,O,P,Q process;
    class J,L memory;
    class Z stop;
    class R success;
```

The growth check includes confirming that the real program break still matches the HMM's tracked break before calling positive `sbrk()`.

## `HmmFree()` flowchart

```mermaid
flowchart TD
    A([HmmFree ptr]) --> B{ptr is NULL?}
    B -- Yes --> Z([Return])
    B -- No --> C[Get block header]
    C --> D{Block already free?}
    D -- Yes --> Z
    D -- No --> E[Mark block free]
    E --> F{Next physical block is free?}
    F -- Yes --> G[Remove next from free list, merge, update physical links]
    F -- No --> H{Previous physical block is free?}
    G --> H
    H -- Yes --> I[Remove previous from free list, merge, update physical links]
    H -- No --> J[Insert merged block at free-list head]
    I --> J
    J --> K{Last physical block is free?}
    K -- No --> Z
    K -- Yes --> L{Real break matches tracked break?}
    L -- No --> Z
    L -- Yes --> M{Only one physical block remains?}

    M -- Yes --> N{Entire owned range is releasable?}
    N -- No --> Z
    N -- Yes --> O[Remove last block from free list]
    O --> P{Negative sbrk succeeds?}
    P -- No --> Q[Restore last block to free list]
    Q --> Z
    P -- Yes --> R[Clear last_physical and tracked break pointers]
    R --> Z

    M -- No --> S{Trailing free block totals at least 8192 bytes?}
    S -- No --> Z
    S -- Yes --> T[Calculate whole 4096-byte chunks to release]
    T --> U[Adjust amount to preserve a valid free tail if needed]
    U --> V{Release is at least 8192 bytes and valid?}
    V -- No --> Z
    V -- Yes --> W{Will the trailing block be removed completely?}
    W -- Yes --> X[Remove trailing block from free list]
    W -- No --> Y[Keep trailing block in free list]
    X --> AA{Negative sbrk succeeds?}
    Y --> AA
    AA -- No --> AB[Restore free-list entry if removed]
    AB --> Z
    AA -- Yes --> AC[Update tail block or last_physical, and tracked break]
    AC --> Z

    %% Color palette
    classDef start fill:#DBEAFE,stroke:#2563EB,stroke-width:2px,color:#1E3A8A;
    classDef decision fill:#FEF3C7,stroke:#D97706,stroke-width:1.7px,color:#78350F;
    classDef process fill:#EDE9FE,stroke:#7C3AED,stroke-width:1.5px,color:#4C1D95;
    classDef memory fill:#CCFBF1,stroke:#0F766E,stroke-width:1.7px,color:#134E4A;
    classDef stop fill:#DCFCE7,stroke:#16A34A,stroke-width:2px,color:#166534;

    class A start;
    class B,D,F,H,K,L,M,N,P,S,V,W,AA decision;
    class C,E,G,I,J,Q,R,AB process;
    class O,T,U,X,Y,AC memory;
    class Z stop;
```

If the entire managed heap is free, `trim_heap()` can return **all** the memory acquired by the HMM, including the initial alignment padding, without applying the 8192-byte threshold. Otherwise, it only returns complete chunks when at least 8192 bytes can be released safely. The diagrams assume `HmmFree()` receives `NULL` or a pointer previously returned by this allocator.
