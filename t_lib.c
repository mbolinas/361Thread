/*
Marc Bolinas and Brian Phillips
CISC361
12/4/2018
*/

/*
IMPORTANT NOTICE:
the order of thread execution is DIFFERENT than in example tests.
this is because our semaphore implementation takes threads out of
both the running and ready queues, while the example tests assume that
the threads stay in the ready queue, but in an 'unrunnable' state.

our semaphore implementation is consistent with what Prof. Shen asked for in
phase 3
 */

#include "t_lib.h"


tcb *running = NULL;
tcb *ready = NULL;

/*
contains a list of all threads currently stuck in sem_wait()
this list is used for threads in send() to find the mailbox of a thread
stuck in a semaphore, because usually a thread in a semaphore is not found
in either running or ready queues
 */
tcb *blockedthreads = NULL;


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
	mbox_create(&(tmp->mbox));

  	getcontext(tmp->thread_context);    /* let tmp be the context of main() */
	running = tmp;
	ready = NULL;
}

void t_create(void (*fct)(int), int id, int pri){

	tcb *tmp;
	tmp = malloc(sizeof(tcb));
	tmp->next = NULL;
	tmp->thread_priority = pri;
	tmp->thread_id = id;
	mbox_create(&(tmp->mbox));
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
	/*
	we assume that sp was already malloc'd in user's code
	*/
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
			/*
			this should never happen unless the example code is bad
			this should only occur due to user error
			*/
			printf("sem_wait() failed! no other thread to switch to\n");
		}
		else if(sp->q == NULL){
			/*
			our semaphore queue is empty, so take running and make it
			the head of semaphore queue
			then swapcontext on:
			head of queue, the currently running thread
			head of ready, the next thread in the queue
			*/
			tcb *old, *new;
			old = running;
			old->next = blockedthreads;
			blockedthreads = old;
			new = ready;
			sp->q = running;
			running = ready;
			ready = ready->next;
			running->next = NULL;
			//sigrelse()
			swapcontext(old->thread_context, new->thread_context);
		}
		else{
			/*
			add running to the end of the semaphore queue
			then swapcontext on:
			tail of queue, the currently running thread
			head of ready, the next thread in the queue
			*/
			tcb *old, *new, *end;
			old = running;
			old->next = blockedthreads;
			blockedthreads = old;
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
	//sighold()

	sp->count = sp->count + 1;
	if(sp->count <= 0){
		tcb *end;
		end = ready;
		tcb *tmp = sp->q;
		/*
		if tmp is NULL, we have no threads in the queue to wake up.
		usually caused by a semaphore initialized with a negative count.
		will segfault without this check
		*/
		if(tmp != NULL){
			/*
			remove tmp from the blockedthreads list
			*/
			if(tmp->thread_id == blockedthreads->thread_id){
				blockedthreads = blockedthreads->next;
			}
			else{
				tcb *tmp_bt = blockedthreads;
				while(tmp_bt->next != NULL){
					if(tmp_bt->next->thread_id == tmp->thread_id){
						tmp_bt->next = tmp_bt->next->next;
					}
				}
			}

			/*
			take the head of the sem queue, insert at end of ready queue
			(or head, if the queue was NULL)
			*/
			sp->q = sp->q->next;
			if(end == NULL){
				end = tmp;
				tmp->next = NULL;
			}
			else{
				while(end->next != NULL)
					end = end->next;
				end->next = tmp;
				tmp->next = NULL;
			}

		}
	}



	//sigrelse()
}

void sem_destroy(sem_t **sp){

	tcb *tmp = (*sp)->q;
	tcb *next;
	/*
	free every tcb in the semaphore's queue.
	we can't just add them into the ready queue because
	it can cause data races.
	*/
	while(tmp != NULL){
		next = tmp->next;
		free(tmp->thread_context->uc_stack.ss_sp);
		free(tmp->thread_context);
		free(tmp);
		tmp = next;
	}

	free(*sp);
	//free(sp);

}

/*
create a new mailbox with each respective field
set to NULL or instantiated to 0
 */
int mbox_create(mbox **mb){
	*mb = malloc(sizeof(mbox));
	(*mb)->msg = NULL;
	(*mb)->mbox_sem = NULL;
	sem_init(&(*mb)->mbox_sem, 0);
	sem_init(&(*mb)->blocksend_sem, 0);
	return 0;
}
/*
destroy each message individually, then call sem_destroy() on
both of the mailbox's semaphores
 */
void mbox_destroy(mbox **mb){
	if((*mb)->msg != NULL){
		message_node *tmp = (*mb)->msg;
		while(tmp != NULL){
			tmp = tmp->next;
			free((*mb)->msg->message);
			free((*mb)->msg);
			(*mb)->msg = tmp;
		}
	}
	if((*mb)->mbox_sem != NULL)
		sem_destroy(&(*mb)->mbox_sem);
	if((*mb)->blocksend_sem != NULL)
		sem_destroy(&(*mb)->blocksend_sem);
	free((*mb));
}

/*
create a new message_node and append it to the end of the 
mailbox list of messages
 */
void mbox_deposit(mbox *mb, char *msg, int len){
	message_node *mn = malloc(sizeof(message_node));
	mn->len = len;
	mn->sender = 0;
	mn->receiver = 0;
	mn->next = NULL;
	mn->message = malloc(sizeof(char) * len);
	strcpy(mn->message, msg);
	//printf("copied: [%s]\n", mn->message);

	if(mb->msg == NULL){
		mb->msg = mn;
	}
	else{
		message_node *tmp = mb->msg;
		while(tmp->next != NULL){
			tmp = tmp->next;
		}
		tmp->next = mn;
	}
}

/*
take a message out of the mailbox, removing the message
from the mailbox's list in the process
 */
void mbox_withdraw(mbox *mb, char *msg, int *len){
	if(mb->msg == NULL){
		*len = 0;
	}
	else{
		*len = mb->msg->len;
		strcpy(msg, mb->msg->message);
		free(mb->msg->message);
		message_node *tmp = mb->msg;
		mb->msg = mb->msg->next;
		free(tmp);
	}
}

/*
find the mailbox belonging to the target thread,
create the message_node and add it to the end of the
mailbox's message list
 */
void send(int tid, char *msg, int len){
	mbox *depositbox = NULL;
	/*
	find the correct mailbox, in either the
	running, ready, or blockedthread lists
	 */
	if(running != NULL && running->thread_id == tid){
		depositbox = running->mbox;
	}
	else{
		tcb *tmp = ready;
		while(tmp != NULL){	/* search through the ready queue */
			if(tmp->thread_id == tid)
				depositbox = tmp->mbox;
			tmp = tmp->next;
		}
		if(depositbox == NULL){	/* search through the blockedthreads list */
			tmp = blockedthreads;
			if(tmp->thread_id == tid)
				depositbox = tmp->mbox;
			tmp = tmp->next;
		}
	}
	/*
	it's possible the user tried sending a message to a
	non-existant thread, meaning there is no mailbox at that TID
	 */
	if(depositbox != NULL){
		message_node *mn = malloc(sizeof(message_node));
		mn->len = len;
		mn->sender = running->thread_id; 
		mn->receiver = tid;
		mn->next = NULL;
		mn->message = malloc(sizeof(char) * len);
		strcpy(mn->message, msg);

		if(depositbox->msg == NULL){	/* set message as the new head */
			depositbox->msg = mn;
		}
		else{	/* append to end */
			message_node *tmp = depositbox->msg;
			while(tmp->next != NULL){
				tmp = tmp->next;
			}
			tmp->next = mn;

		}
		/*
		we only want to signal if there are threads waiting for a receive, aka count < 0
		otherwise, a thread calling sem_wait might not block when in receive()
		 */
		sem_signal(depositbox->mbox_sem);
	}
	else
		printf("(Attempted to send message to thread that does not exist)\n");
}

/*
thread searches through it's own mailbox to find a message sent by TID
if not found, it will wait() until there exists a message by that TID
 */
void receive(int *tid, char *msg, int *len){
	mbox *receivebox = NULL;
	receivebox = running->mbox;
	if(*tid == 0){
		while(receivebox->msg == NULL){	/* not a single message, wait for one */
			sem_wait(receivebox->mbox_sem);
		}
		*len = receivebox->msg->len;
		*tid = receivebox->msg->sender;
		strcpy(msg, receivebox->msg->message);
		receivebox->msg->message = NULL;
		free(receivebox->msg->message);
		message_node *tmp = receivebox->msg;
		receivebox->msg = receivebox->msg->next;
		free(tmp);
		sem_signal(receivebox->blocksend_sem);	/* tell any threads in block_send() to wake */
	}
	else{
		int found = 0;
		while(found == 0){
			message_node *tmp = receivebox->msg;
			if(tmp != NULL && tmp->sender == *tid){
					/*
					we found the message, copy data and exit while(found == 0) loop
					 */
				found = 1;
				*len = tmp->len;
				*tid = tmp->sender;
				strcpy(msg, tmp->message);
				tmp->message = NULL;
				free(tmp->message);
				receivebox->msg = receivebox->msg->next;
				free(tmp);
				sem_signal(receivebox->blocksend_sem);	/* wake up block_send() threads */
			}

			while(found == 0 && tmp != NULL){
				if(tmp->next->sender == *tid){
						/*
						we found the message, copy data and exit while(found == 0) loop
						 */
					found = 1;
					*len = tmp->next->len;
					*tid = tmp->next->sender;
					strcpy(msg, tmp->next->message);
					tmp->next->message = NULL;
					free(tmp->next->message);
					message_node *deleteMe = tmp->next;
					tmp->next = tmp->next->next;
					free(deleteMe);
					sem_signal(receivebox->blocksend_sem);	/* wake up block_send() threads */
				}
				else{
					tmp = tmp->next;
				}
			}
			if(found == 0){
				/*
				message not found, set semaphore->count = 0 to force a wait, and
				call wait() on said semaphore
				 */
				if(receivebox->mbox_sem->count > 0)
					receivebox->mbox_sem->count = 0;
				sem_wait(receivebox->mbox_sem);
			}
		}
	}
}
/*
blocking version of the earlier send() function
blocks via a second semaphore found in each mailbox.
this second semaphore, blocksend_sem, is signaled every time
receive() is called on it's mailbox
 */
void block_send(int tid, char *msg, int len){
	mbox *depositbox = NULL;
	/*
	find the mailbox to send to, just like in send()
	 */
	if(running != NULL && running->thread_id == tid){
		depositbox = running->mbox;
	}
	else{
		tcb *tmp = ready;
		while(tmp != NULL){
			if(tmp->thread_id == tid)
				depositbox = tmp->mbox;
			tmp = tmp->next;
		}
		if(depositbox == NULL){
			tmp = blockedthreads;
			if(tmp->thread_id == tid)
				depositbox = tmp->mbox;
			tmp = tmp->next;
		}
	}
	if(depositbox != NULL){
		/*
		create the message and append to end of list, just like in send()
		 */
		message_node *mn = malloc(sizeof(message_node));
		mn->len = len;
		mn->sender = running->thread_id; 
		mn->receiver = tid;
		mn->next = NULL;
		mn->message = malloc(sizeof(char) * len);
		strcpy(mn->message, msg);
		if(depositbox->msg == NULL){
			depositbox->msg = mn;
		}
		else{
			message_node *tmp = depositbox->msg;
			while(tmp->next != NULL){
				tmp = tmp->next;
			}
			tmp->next = mn;
		}
		/*
		we only want to signal if there are threads waiting for a receive, aka count < 0
		otherwise, a thread calling sem_wait might not block when in receive()
		 */
		sem_signal(depositbox->mbox_sem);
		while(mn != NULL){
			sem_wait(depositbox->blocksend_sem);
			if(mn->len != len){	
				/*
				the message has been read if the other thread deleted it
				meaning that the lengths will be different because it will no longer exist
				 */
				mn = NULL;
			}
		}
	}
	else
		printf("(Attempted to send message to thread that does not exist)\n");
}

/*
receive() is blocking by nature, so block_receive() functions exactly like
regular receive()
 */
void block_receive(int *tid, char *msg, int *len){
	receive(tid, msg, len);
}




