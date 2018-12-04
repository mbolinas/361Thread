/* 
 * Test Program #3 - Semaphore
 */

#include <stdio.h>
#include <stdlib.h>
#include "ud_thread.h"

int main(void) {

   int i;

   t_init();
   
   mbox *mybox;
   mbox_create(&mybox);

   t_shutdown();

   return 0;
}