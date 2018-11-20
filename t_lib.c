/*
Marc Bolinas and Brian Phillips
CISC361
11/19/2018
*/

#include "t_lib.h"


tcb *running = NULL;
tcb *ready = NULL;



void t_yield(){

	if(running != NULL && ready != NULL){
		tcb *end;
		end = ready;
		while(end->next != NULL)
			end = end->next;
		

		tcb *old, *new;
		old = running;
		new = ready;

		end->next = running;
		running = ready;
		ready = ready->next;
		running->next = NULL;

		swapcontext(old->thread_context, new->thread_context);
	}
}

void t_init(){

	tcb *tmp;
	tmp = malloc(sizeof(tcb));
	tmp->thread_priority = 0;
	tmp->thread_id = 0;
	tmp->next = NULL;
	tmp->thread_context = malloc(sizeof(ucontext_t));


  	getcontext(tmp->thread_context);    /* let tmp be the context of main() */
	running = tmp;
	ready = NULL;
}

int t_create(void (*fct)(int), int id, int pri){

	tcb *tmp;
	tmp = malloc(sizeof(tcb));
	tmp->next = NULL;
	tmp->thread_priority = pri;
	tmp->thread_id = id;

	tcb *end;
	end = ready;

	//append to the end of the ready queue, or initialize it if it's empty
	if(end != NULL){
		while(end->next != NULL)
			end = end->next;

		end->next = tmp;
	}
	else
		ready = tmp;


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

	return 0;
}

void t_shutdown(){
	//free everything in running, a pointer, and ready, a list	
    free(running->thread_context->uc_stack.ss_sp);
    free(running->thread_context);
    free(running);

    tcb *iterator;
    iterator = ready;
    while(iterator != NULL){
    	tcb *tmp;
    	tmp = iterator;

    	free(tmp->thread_context->uc_stack.ss_sp);
    	free(tmp->thread_context);
    	iterator = iterator->next;
    	free(tmp);

    }
}

void t_terminate(){
	if(ready == NULL){
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
		pick a thread off of ready to run next
		point running to that thread and then context switch
		*/

		free(running->thread_context->uc_stack.ss_sp);
		free(running->thread_context);
		free(running);

		running = ready;
		ready = ready->next;
		running->next = NULL;



		setcontext(running->thread_context);

	}
}

int sem_init(sem_t **sp, int sem_count){
	*sp = malloc(sizeof(sem_t));
	(*sp)->count = sem_count;
	(*sp)->q = NULL;

	return 0;
}

void sem_wait(sem_t *sp){
	//sighold()

	sp->count = sp->count - 1;
	if(sp->count < 0){
		if(ready == NULL){
			//this should never happen unless the example code is bad
			//this should only occur due to user error
			printf("sem_wait() failed! no other thread to switch to\n");

		}
		else if(sp->q == NULL){
			tcb *old, *new;
			old = running;
			new = ready;
			sp->q = running;
			running = ready;
			ready = ready->next;
			running->next = NULL;
			//sigrelse()
			swapcontext(old->thread_context, new->thread_context);
		}
		else{
			tcb *old, *new, *end;
			old = running;
			new = ready;
			end = sp->q;
			while(end->next != NULL)
				end = end->next;

			end->next = running;
			running = ready;
			ready = ready->next;
			running->next = NULL;

			//sigrelse()
			swapcontext(old->thread_context, new->thread_context);

		}
	}
	//sigrelse()
}

void sem_signal(sem_t *sp){
	//stop interrupts

	sp->count = sp->count + 1;
	if(sp->count <= 0){
		//remove a process from sp->queue
		//place process onto the ready list
			//moves the first thread in sp->queue to the end of ready
			//re-enable interrupts
			//finish
	}



	//allow interrupts	
}

void sem_destroy(sem_t **sp){

	tcb *tmp = (*sp)->q;
	tcb *next;

	while(tmp != NULL){
		next = tmp->next;
		free(tmp->thread_context.uc_stack.ss_sp);
		free(tmp->thread_context);
		free(tmp);
		tmp = next;
	}

	free(*sp);
	free(sp);

}














