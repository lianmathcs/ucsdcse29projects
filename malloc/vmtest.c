
#include <stdio.h>
#include <assert.h>
#include "vmlib.h"
#include <stdint.h>
#include "vm.h" 
#include <stdlib.h>

int main()
{
   //  vminit(2000); // comment this out if using vmload()
    /* alternatively, load a heap image */
   vmload("tests/img/many_free.img");
   // vmload("tests/img/last_free.img");
   //  vmload("tests/img/no_free.img");
    
    void *ptr1 = vmalloc(20);

    struct block_header *hdr = (struct block_header *)((char*)ptr1 - 8);
    void *ptr2 = vmalloc(670);
    void *ptr3 = vmalloc(500);
   // void *ptr3 = vmalloc(18);
       //void *ptr5 = vmalloc(1000);
   vmfree(ptr1);	
  //  vmfree(ptr1); 
  //  void *ptr4 = vmalloc(2000);
 //  assert(hdr->size_status == 19);
   void *ptr = vmalloc(112);
  vmfree(ptr);	
    
  // vmfree(ptr2);
   vminfo(); // print out how the heap looks like at this point in time for
              // easy visualization

//    vmdestroy(); // frees all memory allocated by vminit() or vmload()
    return 0;

}