# Dynamic-Memory-Allocation-Library
Implementation of a library that allows for dynamic allocation and freeing of memory in C.

To include it at the top of your C file :
```c
#include "dynamicmemory.h"
```
To allocate memory you have to call:
```c
allocateMemory(size_t memory_size)
```

To free the piece of memory you have to call:

```c
freeMemory(void *memory_location);
````
