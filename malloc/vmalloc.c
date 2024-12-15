#include "vm.h"
#include "vmlib.h"
#include <stdio.h>
#include <stdint.h>

struct block_header*get_next_block(struct block_header *block){
	uint64_t block_size = BLKSZ(block);
	struct block_header*next = (struct block_header*)((char *)block + block_size);
	return next;
}

void *vmalloc(size_t size)
{ 
	if(size <= 0){
		return NULL;
	}

	uint64_t total_size = size + 8;         		/* 8 = sizeof(struct block_header) */
	uint64_t size_rounded = ROUND_UP(total_size,16); 
	if(total_size < 16 ) total_size = 16;	
	
	struct block_header * best_fit_ptr = NULL;
	struct block_header *heap_ptr = heapstart;    
	
     while (heap_ptr != NULL) {
        uint64_t blocksz = BLKSZ(heap_ptr);
        if(blocksz == 0) break;
        if(((heap_ptr->size_status & 1) == 0) && (blocksz >= size_rounded)) {            
            if(best_fit_ptr == NULL || blocksz < BLKSZ(best_fit_ptr) ) {
                best_fit_ptr = heap_ptr;
            }            
        }
        heap_ptr = get_next_block(heap_ptr);
    }
	
	if(best_fit_ptr ==NULL) return NULL; 

	uint64_t blocksz = BLKSZ(best_fit_ptr);
    uint64_t remaining_size = blocksz - size_rounded;

	 if (remaining_size >= 16) {
		struct block_header *next_block = (struct block_header *)((char *)best_fit_ptr + size_rounded);
        next_block->size_status = remaining_size & VM_BLKSZMASK;
        best_fit_ptr->size_status = size_rounded | VM_BUSY | VM_PREVBUSY;
	 }
	 else {
        best_fit_ptr->size_status |= VM_BUSY;
	 }	    
		return (void *)((char *)best_fit_ptr + 8);
}


     // free - set lsb = 0;( free)  ;  conbine the two free or other ( allocate the block, update the footer)  
     // 2 bits to control values ; 1 , and one before the block is free or not ; ( update the both current and prev-block free or not
     // size_status  51 bytes, 51-48 =3   ,block is not free   size_status stores the size + other values 
     // 51 = 110011  ( the last two bits 11 , 1 is busy ,prev 1 is busy 
     // 47 + 8 = 55 ; 16 * 4 = 64 ( 64 > 55)  
     // (struct blokc_header*) (char *) block + blocksz  ( char = 1 byte + 48 ) = move this to the next block in the heap
     //  cast back the struct * ...