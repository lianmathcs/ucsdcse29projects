#include "vm.h"
#include "vmlib.h"

/**
 * The vmfree() function frees the memory space pointed to by ptr,
 * which must have been returned by a previous call to vmmalloc().
 * Otherwise, or if free(ptr) has already been called before,
 * undefined behavior occurs.
 * If ptr is NULL, no operation is performed.
 */

void vmfree(void *ptr) {
    if (ptr == NULL) return;  // If ptr is NULL, no operation is performed.

    struct block_header *curr_block = (struct block_header *)((char *)ptr - sizeof(struct block_header));

    if ((curr_block->size_status & VM_BUSY) == 0) return;

    curr_block->size_status &= ~VM_BUSY;  // mark as free 

    struct block_footer *curr_footer = (struct block_footer *)((char *)curr_block + BLKSZ(curr_block) - sizeof(struct block_footer));
    curr_footer->size = BLKSZ(curr_block);

    // merge previous block if it's free.
    if ((char *)curr_block > (char *)heapstart) { 
        struct block_footer *prev_footer = (struct block_footer *)((char *)curr_block - sizeof(struct block_footer));
        uint64_t prev_block_size = prev_footer->size & VM_BLKSZMASK;

        if (prev_block_size >= 16) {
            struct block_header *prev_free_block = (struct block_header *)((char *)curr_block - prev_block_size);

            // free + large enough 
            if ((prev_free_block->size_status & VM_BUSY) == 0 && BLKSZ(prev_free_block) >= 16) {
                uint64_t merged_size = prev_block_size + BLKSZ(curr_block);

                if (merged_size >= 16) {
                    prev_free_block->size_status = (prev_free_block->size_status & ~VM_BLKSZMASK) | (merged_size & VM_BLKSZMASK);

                    struct block_footer *merged_footer = (struct block_footer *)((char *)prev_free_block + merged_size - sizeof(struct block_footer));
                    merged_footer->size = merged_size;

                    curr_block = prev_free_block;
                }
            }
        }
    }

    //  merge  the next block if it's free.
    struct block_header *next_block = (struct block_header *)((char *)curr_block + BLKSZ(curr_block));
  
    if ((next_block->size_status & VM_BUSY) == 0) {  // Check if next block is free
        uint64_t next_size = BLKSZ(next_block);   
        curr_block->size_status = (curr_block->size_status & ~VM_BLKSZMASK) | (BLKSZ(curr_block) + next_size);
        
        struct block_footer *merged_footer = (struct block_footer *)((char *)curr_block + BLKSZ(curr_block) + next_size - sizeof(struct block_footer));
        merged_footer->size = (BLKSZ(curr_block) + next_size);
    }
}