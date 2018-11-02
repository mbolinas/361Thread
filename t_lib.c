#include "t_lib.h"


tcb *running = NULL;
tcb *ready = NULL;



void t_yield(){
	if(running != NULL && ready != NULL){
		tcb *end;
		end = ready;
		while(end->next != NULL){
			end = end->next;
		}

		tcb *old;
		tcb *new;
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
	if(end != NULL){
		while(end->next != NULL){
			end = end->next;
		}
		end->next = tmp;
	}
	else{
		ready = tmp;
	}
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
		//what happens when we terminate the only thread remaining?
		//there's nothing to switch to
		//does this function just like a t_shutdown()?
		//if so, then this will free all pointers in ready
		//but not do any context switching
	}
	else{
		free(running->thread_context->uc_stack.ss_sp);
		free(running->thread_context);
		free(running);

		running = ready;
		ready = ready->next;
		running->next = NULL;



		setcontext(running->thread_context);

	}



}







