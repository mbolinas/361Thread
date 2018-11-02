#include "t_lib.h"

//ucontext_t *running;
//ucontext_t *ready;

tcb *running = NULL;
tcb *ready = NULL;



void t_yield(){
	
	if(running != NULL && ready != NULL){
		tcb *end;
		end = ready;
		while(end->next != NULL){
			end = end->next;
		}

		end->next = running;
		running = ready;
		ready = ready->next;
		running->next = NULL;

		swapcontext(ready->thread_context, running->thread_context);
	}



/*	ucontext_t *tmp;

	tmp = running;
	running = ready;
	ready = tmp;

	swapcontext(ready, running);*/


}

void t_init(){

	tcb *tmp;
	tmp = malloc(sizeof(tcb));
	tmp->thread_priority = 0;
	tmp->thread_id = 0;
	tmp->next = NULL;
	tmp->thread_context = malloc(sizeof(ucontext_t));

	//tmp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t));

  	getcontext(tmp->thread_context);    /* let tmp be the context of main() */
	running = tmp;
	ready = NULL;
	//printf("running: id= %d\n", running->thread_id);

	//printf("init finished\n");
}

int t_create(void (*fct)(int), int id, int pri){

	tcb *tmp;
	tmp = malloc(sizeof(tcb));
	tmp->next = NULL;
	tmp->thread_priority = pri;
	tmp->thread_id = id;

	printf("nice\n");

	tcb *end;
	end = ready;
	if(end != NULL){
		while(end->next != NULL){
			end = end->next;
		}
		end->next = tmp;
	}
	else{
		//printf("first thread added to ready!\n");
		ready = tmp;
	}

	printf("beep\n");

	size_t sz = 0x10000;

	//ucontext_t *uc;

	ucontext_t *tmp_ucon;

	tmp_ucon = malloc(sizeof(ucontext_t));

	//tmp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t));

	printf("???\n");

	getcontext(tmp_ucon);

	printf("yeet\n");

	tmp_ucon->uc_stack.ss_sp = mmap(0, sz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, -1, 0);

	tmp_ucon->uc_stack.ss_sp = malloc(sz);  /* new statement */
	tmp_ucon->uc_stack.ss_size = sz;
	tmp_ucon->uc_stack.ss_flags = 0;

	printf("uh what\n");

	tmp_ucon->uc_link = running->thread_context; 
	makecontext(tmp_ucon, (void (*)(void)) fct, 1, id);

	tmp->thread_context = tmp_ucon;


	printf("create finished\n");
	//ready = uc;

	return 0;
}

void t_terminate(){

}