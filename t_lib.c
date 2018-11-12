/*
Marc Bolinas and Brian Phillips
CISC361

11/2/2018

*/

#include "t_lib.h"


tcb *running = NULL;
tcb *readyhigh = NULL;
tcb *readylow = NULL;



void t_yield(){
	if(readyhigh != NULL || readylow != NULL){
		ualarm(0, 0);
		sighold(14);
		tcb *old, *new, *end;
		old = running;

		if(old->thread_priority == 0){
			end = readyhigh;
			if(end == NULL){
				readyhigh = old;
			}
			else{
				while(end->next != NULL)
					end = end->next;
				end->next = old;
			}

			
		}
		else{
			end = readylow;
			if(end == NULL){
				readylow = old;
			}
			else{
				while(end->next != NULL)
					end = end->next;
				end->next = old;
			}	

		}
		if(readyhigh == NULL){
			new = readylow;
			readylow = readylow->next;
		}
		else{
			new = readyhigh;
			readyhigh = readyhigh->next;
		}



		running = new;
		running->next = NULL;

		sigrelse(14);
		ualarm(1000, 0);
		swapcontext(old->thread_context, new->thread_context);

	}







}

void t_init(){
	sighold(14);
	tcb *tmp;
	tmp = malloc(sizeof(tcb));
	tmp->thread_priority = 1;
	tmp->thread_id = 0;
	tmp->next = NULL;
	tmp->thread_context = malloc(sizeof(ucontext_t));


  	getcontext(tmp->thread_context);    /* let tmp be the context of main() */
	running = tmp;
	readyhigh = NULL;
	readylow = NULL;

	signal(SIGALRM, force_yield);
	ualarm(1000, 0);
	sigrelse(14);
}

int t_create(void (*fct)(int), int id, int pri){
	sighold(14);
	tcb *tmp;
	tmp = malloc(sizeof(tcb));
	tmp->next = NULL;
	tmp->thread_priority = pri;
	tmp->thread_id = id;

	tcb *end;

	if(pri == 0){
		end = readyhigh;
		if(end != NULL){
			while(end->next != NULL)
				end = end->next;

			end->next = tmp;
		}
		else{
			readyhigh = tmp;
		}
	}
	else{
		end = readylow;
		if(end != NULL){
			while(end->next != NULL)
				end = end->next;

			end->next = tmp;
		}
		else{
			readylow = tmp;
		}
	}

	//setting context of stack
	size_t sz = 0x10000;
	ucontext_t *tmp_ucon;
	tmp_ucon = malloc(sizeof(ucontext_t));
	getcontext(tmp_ucon);
	tmp_ucon->uc_stack.ss_sp = mmap(0, sz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);
	tmp_ucon->uc_stack.ss_sp = malloc(sz);  /* new statement */
	tmp_ucon->uc_stack.ss_size = sz;
	tmp_ucon->uc_stack.ss_flags = 0;
	tmp_ucon->uc_link = running->thread_context; 
	makecontext(tmp_ucon, (void (*)(void)) fct, 1, id);


	tmp->thread_context = tmp_ucon;
	sigrelse(14);
	return 0;
}

void t_shutdown(){
	//free everything in running, a pointer, and ready, a list	
	sighold(14);
    free(running->thread_context->uc_stack.ss_sp);
    free(running->thread_context);
    free(running);

    tcb *iterator;
    iterator = readyhigh;
    while(iterator != NULL){
    	tcb *tmp;
    	tmp = iterator;

    	free(tmp->thread_context->uc_stack.ss_sp);
    	free(tmp->thread_context);
    	iterator = iterator->next;
    	free(tmp);

    }
    iterator = readylow;
    while(iterator != NULL){
    	tcb *tmp;
    	tmp = iterator;

    	free(tmp->thread_context->uc_stack.ss_sp);
    	free(tmp->thread_context);
    	iterator = iterator->next;
    	free(tmp);

    }
    sigrelse(14);
}

void t_terminate(){
	sighold(14);
	if(readyhigh == NULL && readylow == NULL){
		/*
		what happens when we terminate the only thread remaining?
		there's nothing to switch to
		does this function just like a t_shutdown()?
		if so, then this will free all pointers in ready
		but not do any context switching
		*/
	}
	else{
		/*
		free the currently running thread
		pick a thread off of readyhigh or readylow to run next
		point running to that thread and then context switch
		*/

		free(running->thread_context->uc_stack.ss_sp);
		free(running->thread_context);
		free(running);

		running = readyhigh;
		if(readyhigh != NULL){
			running = readyhigh;
			readyhigh = readyhigh->next;
			running->next = NULL;
		}
		else{
			running = readylow;
			readylow = readylow->next;
			running->next = NULL;
		}




		setcontext(running->thread_context);

	}
	sigrelse(14);


}

void force_yield(){
	printf("forcing yield...\n");
	fflush(stdout);
	t_yield();
}







