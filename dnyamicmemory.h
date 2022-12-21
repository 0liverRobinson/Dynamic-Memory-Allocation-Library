#include <stddef.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <math.h>

#define BLOCK_SIZE 8192
#define FREELIST_SIZE 0X18
#define MAX_BLOCK_SIZE  BLOCK_SIZE - FREELIST_SIZE
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)

typedef struct FreeList
{
    short len;
    bool free;
    struct FreeList *next;
} FreeList;

FreeList *freelist = NULL;
void *heap_watermark;

/**
 * @brief Allocate memory and return base address of allocated memory to user.
 * 
 * @param memory_size Size of memory wanting to be allocated.
 * @return void* The start address of new allocated memory.
 */
void *allocateMemory(size_t memory_size)
{

    // Round memory size to nearest power of 2
    memory_size--;
    memory_size |= memory_size >> 1;
    memory_size |= memory_size >> 2;
    memory_size |= memory_size >> 4;
    memory_size |= memory_size >> 8;
    memory_size |= memory_size >> 16;
    memory_size++;


    // Initialise the freelist
    if (freelist == NULL)
    {

        // Dedicate area of memory
        freelist = sbrk(BLOCK_SIZE);

        // Set specs
        freelist->free = false;
        freelist->len = memory_size;

        // Get new top of heap
        heap_watermark = sbrk(0);
        
        // Get pointer to free list
        void* end = freelist;

        // Point to the end of free list and start of data
        end += FREELIST_SIZE;

        // Return the pointer
        return (void *)(end);
    }
    
    // If it's larger than our max size, allocate a page
    if ( memory_size >= MAX_BLOCK_SIZE )
    {
        // Get needed amount of pages
        int PAGE_COUNT = (int) ceil( memory_size/PAGE_SIZE );

        // If the memory size is perfect fit for n page sizes, increase by 1
        if (memory_size % PAGE_SIZE  == 0 )
            PAGE_COUNT++;

        // Calculate total page size
        size_t TOTAL_PAGE_SIZE = PAGE_COUNT * PAGE_SIZE;

        // Allocate page(s) and return pointer
        return mmap(NULL, TOTAL_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    }


    FreeList* list_traverser = freelist;

    for ( ; list_traverser->next != NULL; list_traverser = list_traverser->next) {

        // If we find a free item in the list that will fit our data
        if (list_traverser->free && list_traverser->len >= memory_size ) {
            
            // Get pointer location of start of data of freelist link
            void *list_pointer = list_traverser;
            list_pointer += FREELIST_SIZE;

            // Set freelist link to false
            list_traverser->free = false;

            // Return pointer
            return list_pointer;
        }
    }

    // Get the addr of the last item in the free list
    void *end = list_traverser;
    
    // Move to the hypothetical end of our new item if it were added
    end += list_traverser->len;
    end += FREELIST_SIZE * 2;
    end += memory_size;

    // Check too see if we come out of bounds
    if ( end >= heap_watermark)
    {
        // Take data off the heap and get start of new data section
        void *new_addr = sbrk(BLOCK_SIZE);

        // Create new freelist link at new allocated heap space
        FreeList *new_list = new_addr;
        new_list->free = false;
        new_list->len = memory_size;
        
        // Move to start of data section
        new_addr += FREELIST_SIZE;

        // Set the new head of the freelist to this link
        list_traverser->next = new_list;

        // Get a new heap watermark
        heap_watermark = sbrk(0);

        // Return address of start of data section
        return new_addr;
    } 
    else 
    {
        // Get the end of the last item in the freelist 
        void* end = list_traverser;
        end += FREELIST_SIZE;
        end += list_traverser->len;

        // Create new freelist link
        FreeList *new_fl = (end);
        new_fl->free = false;
        new_fl->next = NULL;
        new_fl->len = memory_size;

        // Set new_item the new head of the freelist
        list_traverser->next = new_fl;

        // Return the location of the start of the data (past freelist)
        return (void *)(end + FREELIST_SIZE);
    }
}

/**
 * @brief Will free memory allocated previously.
 * 
 * @param memory_location The piece of memory you want to free
 */
void freeMemory(void *memory_location)
{
    // Get the position of the start of the free list
    void *currentFreeListLocation = memory_location - FREELIST_SIZE;

    // Get the freelsit link this memory address is associated with
    FreeList *current_freelist = currentFreeListLocation;

    // Set it to free
    current_freelist->free = true;
}