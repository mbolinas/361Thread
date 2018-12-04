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
   printf("box created\n");
   char wow[] = "wow";
   mbox_deposit(mybox, &wow, 3);
   mbox_deposit(mybox, &wow, 3);
   mbox_deposit(mybox, &wow, 3);

   mbox_withdraw(mybox, &wow, &i);
   printf("withdrew: %s\n", &wow);
      mbox_withdraw(mybox, &wow, &i);
   printf("withdrew: %s\n", &wow);
      mbox_withdraw(mybox, &wow, &i);
   printf("withdrew: %s\n", &wow);

   mbox_destroy(&mybox);
   printf("box destroyed\n");
   t_shutdown();

   return 0;
}