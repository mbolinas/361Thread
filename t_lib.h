#include <stdio.h>
#include <stdlib.h>
#include <sys/ucontext.h>
#include <sys/mman.h>

//#include "ud_thread.h"

struct tcb{
	int thread_id;
	int thread_priority;
	ucontext_t *thread_context;
	struct tcb *next;
};

typedef struct tcb tcb;