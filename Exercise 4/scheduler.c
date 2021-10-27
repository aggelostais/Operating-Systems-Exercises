#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2			       /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define EXEC_CHAR_LIM 10

struct Node {
	struct Node *next;
	int id; //scheduler defined id
	int pid; //process pid
	char name [EXEC_CHAR_LIM]; //onoma ektelesimou
};
struct Queue {
	struct Node *front;
	struct Node *last;
	unsigned int size;
	unsigned int next_id; //next_id to be given if a new process arrives
};

void init(struct Queue *q) {
	q->front = NULL;
	q->last = NULL;
	q->size = 0;
	q->next_id=1;
}
struct Node * front(struct Queue *q) {
	return q->front;
}
int next_id(struct Queue *q){
	return q->next_id;
}
void increase_next_id(struct Queue *q){
	q->next_id++;
}
void pop(struct Queue *q) { //Eksagogi diergasias pou einai sto kefali tis ouras
	q->size--;
	struct Node *tmp = q->front;
	q->front = q->front->next;
	free(tmp);
}
void push(struct Queue *q, int id, int pid, char exe []) { //Eisagogi diergasias sto telos tis ouras
	q->size++;
	if (q->front == NULL) {  //An i oura itan adeia balto sti kefali
		q->front = (struct Node *) malloc(sizeof(struct Node));
		q->front->id = id;
		q->front->pid= pid;
		strcpy(q->front->name,exe); //Den epitrepetai anathesi alla antigrafi
		q->front->next = NULL;
		q->last = q->front;
	} 
	else {
		q->last->next = (struct Node *) malloc(sizeof(struct Node));
		q->last->next->id= id;
		q->last->next->next = NULL;
		q->last->next->pid= pid;
		strcpy(q->last->next->name,exe);
		q->last = q->last->next;
	}
}
void print_queue(struct Queue *q){
	struct Node *tmp = q->front;
	printf("Processes queue: ");
	while (tmp != NULL) {
		printf("%d (pid:%d, name:%s) --> ", tmp->id,tmp->pid,tmp->name);
		tmp = tmp->next;
	}
	printf("NULL\n");
}
//Pairnei gia orisma to pid tis diergasias pou tha afairethei
void remove_from_queue(struct Queue *q, int pid)
{
	struct Node *curr = q->front; //Arxizo apo to front kai psaxno to kombo diagrafis
	if(curr != NULL)
	{
		if(curr->pid == pid){//node to be removed is front
			q->front = q->front->next;
			free(curr);
			q->size--;
			return;
		}
	}
	struct Node *prev = q->front;
	curr = curr->next;
	while(curr != NULL) //Aanzitisi komvou diagrafis
	{
		if(curr->pid == pid) 
			break;
		else{
			curr = curr->next;
			prev = prev->next;
		}
	}
	if(curr == NULL) return;
	else{ //Brethike o komvos diagrafis
		prev->next = curr->next;
		curr->next = NULL;
		free(curr);
		q->size--;
		//next 3 lines are to restore q->last, na deixnei sto sosto teleutaio komvo
		struct Node *tmp = q->front; 
		while(tmp->next != NULL) tmp = tmp->next;
		q->last = tmp;
		return;
	}
}
int empty(struct Queue *q) {
	return q->size == 0;
}
//Euresi tou pid tis diergasias apo to id
int find_pid(struct Queue *q, int id) 
{
	struct Node *curr = q->front; //Arxizo apo to front kai psaxno to kombo
	if(curr != NULL){
		if(curr->id == id){//node is front
			return curr->pid;
		}
	}
	curr = curr->next;
	while(curr != NULL){
		if(curr->id == id) break;
		else curr = curr->next;
	}
	if(curr == NULL) return -1;
	else//Brethike o komvos 
		return curr->pid;
}
//Euresi komvou apo to pid
struct Node * find_node_from_pid(struct Queue *q, int search_pid)
{
	struct Node *curr = q->front; //Arxizo apo to front kai psaxno to kombo
	while(curr != NULL){
		if(curr->pid == search_pid) break;
		else curr = curr->next;
	}
	if(curr == NULL) return NULL;
	else//Brethike o komvos 
		return curr;
}

struct Queue q;

/*
 * SIGALRM handler
 */
//Stamataei ti diergasia kefali tis ouras
static void
sigalrm_handler(int signum)
{
	// printf("alarm handler used\n");
	pid_t to_stop = (front(&q)->pid); //Pairnei to pid tis diergasias kefalis
	printf("Scheduler: Stopping (id:%d, pid:%d, name:%s).\n",
			front(&q)->id,front(&q)->pid,front(&q)->name);
	kill(to_stop, SIGSTOP);
}

/*
 * SIGCHLD handler
 */
//Kalitai kathe fora pou lambanetai sima termatismou-pausis opoioudipote paidiou
static void
sigchld_handler(int signum)
{
	// printf("sigchld handler used\n");
	pid_t p;
	int status;

	if (signum != SIGCHLD) {
		fprintf(stderr, "Internal error: Called for signum %d, not SIGCHLD\n",
			signum);
		exit(1);
	}

	/*
	 * Something has happened to one of the children.
	 * We use waitpid() with the WUNTRACED flag, instead of wait(), because
	 * SIGCHLD may have been received for a stopped, not dead child.
	 *
	 * A single SIGCHLD may be received if many processes die at the same time.
	 * We use waitpid() with the WNOHANG flag in a loop, to make sure all
	 * children are taken care of before leaving the handler.
	 */

	for (;;) {
		p = waitpid(-1, &status, WUNTRACED | WNOHANG); //epistrefei an opoiodipote paidi termatistei i stamatisei na ekteleitai
		if (p < 0) {
			perror("waitpid");
			exit(1);
		}
		if (p == 0)
			break;

		explain_wait_status(p, status);
		// if (WIFEXITED(status) || WIFSIGNALED(status))
		if (WIFEXITED(status)) {
			/* A child has died normally */
			printf("Scheduler: (id:%d, pid:%d, name:%s) terminated normally.\n",
			find_node_from_pid(&q,p)->id,find_node_from_pid(&q,p)->pid,find_node_from_pid(&q,p)->name);
			pop(&q); // remove child from queue
		}
		if(WIFSIGNALED(status)){
			//a child has died by signal
			printf("Scheduler: (id:%d, pid:%d, name:%s) has been killed.\n",
					find_node_from_pid(&q,p)->id,find_node_from_pid(&q,p)->pid,find_node_from_pid(&q,p)->name);
			//if(p != (front(&q)->pid)){
			remove_from_queue(&q, p);
			//if(empty(&q)){
			//	fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
			//		exit(1);
			//	}
				//return;
			//}
		}
		if (WIFSTOPPED(status)) {
			/* A child has stopped due to SIGSTOP/SIGTSTP, etc... */
			if(p==front(&q)->pid){
				push(&q,front(&q)->id,front(&q)->pid,front(&q)->name); //bale ti diergasia sto telos tis ouras
				pop(&q); // remove process from queue
			//move stopped process at the end of the queue
			}
		}
		if(empty(&q)){
			fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
			exit(1);
		}
		alarm(SCHED_TQ_SEC); //set alarm
		printf("Scheduler: Activating (id:%d, pid:%d, name:%s).\n",
				front(&q)->id,front(&q)->pid,front(&q)->name);
		kill((front(&q)->pid), SIGCONT); //send SIGCONT to next process in queue, stin arxi tis ouras
	}
}

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset; //a collection of values for signals that are used in various system calls. 
	struct sigaction sa; //Orizei to struct sigaction sa
	sa.sa_handler = sigchld_handler; //Periptosi SIGCHLD 
	sa.sa_flags = SA_RESTART; 
	sigemptyset(&sigset); //Creates empty sigset
	sigaddset(&sigset, SIGCHLD); 
	sigaddset(&sigset, SIGALRM); 
	sa.sa_mask = sigset; //O handler blockarei pros to paron ta simata SIGCHILD kai SIGALRM otan ekteleitai
	if (sigaction(SIGCHLD, &sa, NULL) < 0) //Gia lipsi simatos SIGCHLD ektelese to handler mas
	{ 
		perror("sigaction: sigchld");
		exit(1);
	}
	sa.sa_handler = sigalrm_handler; //Periptosi SIGALRM
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}


int main(int argc, char *argv[])
{
	int nproc;
	init(&q); //Dimiourgei mia keni oura
	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	//Einai ta programmata pou tha xronodromologithoun
	 int i;
	 for(i = 1; i < argc; i++)
	 {
		 char *executable = argv[i]; //onoma programmatos pou tha xronodromologithei
	 	 char *newargv[] = { argv[i], NULL, NULL, NULL }; //oi parametroi pou tha parei to execve, 
		  												  //me to proto na einai to onoma tou ektelesimou
	 	 char *newenviron[] = { NULL }; //perivallon ektelesis
		 pid_t p = fork();	//gia kathe programma pou dinetai san orisma dimiourgei mia diergasia
		 if(p < 0){
			 perror("error: fork");
			 exit(1);
		 }
		 else if(p == 0){ //an eisai i diergasia paidi 
			 raise(SIGSTOP); //child process raises SIGSTOP immediately
			 				//prota dimiourgountai oles oi diergasies 
							 //Otan dothei SIGCONT i kathe mia tha sinexisei apo edo
			 execve(executable, newargv, newenviron); //i kathe diergasia paidi ektelei to antistoixo programma
			 perror("execve");
			 exit(1);
		 }
		 else{ //an eisai i diergasia pateras
			 push(&q,next_id(&q),p,executable); //Scheduler pushes child's pid to queue
			 increase_next_id(&q);
		 }
	 }

	nproc = argc - 1; /* number of processes created*/		
					  //argc-1 giati i proti parametros einai to onoma tou arxikou ektelesimou
	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc); //perimenei na termatisoun ola ta paidia
	printf("All initialized correctly. Please proceed.\n");
	print_queue(&q);	//typonei tin oura ton programmaton pou prokeitai na xronodromologithoun

	// Install SIGALRM and SIGCHLD handlers.
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}

	alarm(SCHED_TQ_SEC); //set an alarm for SCHED_TQ_SEC
	printf("Scheduler: Activating (id:%d, pid:%d, name:%s).\n",front(&q)->id,front(&q)->pid,front(&q)->name); 
	kill((front(&q)->pid), SIGCONT); //send SIGCONT to first process
	//energopoioume ti proti diergasia gia na arxisei tin ektelesi tis
	
	// loop forever  until we exit from inside a signal handler.
	while (pause())
		;
	// Unreachable
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
