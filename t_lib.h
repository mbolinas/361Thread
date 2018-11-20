#include <stdio.h>
#include <stdlib.h>
#include <sys/ucontext.h>
#include <sys/mman.h>
#include <signal.h>

//#include "ud_thread.h"

void t_create(void (*fct)(int), int id, int pri);
void t_yield();
void t_init();
void t_terminate();
void t_shutdown();
void force_yield();


struct tcb{
	int thread_id;
	int thread_priority;
	ucontext_t *thread_context;
	struct tcb *next;
};

typedef struct tcb tcb;

struct sem_t{
	int count;
	tcb *q;
};

typedef struct sem_t sem_t;