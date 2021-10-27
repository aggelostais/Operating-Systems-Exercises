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
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */
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
void pop(struct Queue *q) {
	q->size--;
	struct Node *tmp = q->front;
	q->front = q->front->next;
	free(tmp);
}
void push(struct Queue *q, int id, int pid, char exe []) {
	q->size++;
	if (q->front == NULL) {  //If the queue was empty
		q->front = (struct Node *) malloc(sizeof(struct Node));
		q->front->id = id;
		q->front->pid= pid;
		strcpy(q->front->name,exe);
		q->front->next = NULL;
		q->last = q->front;
	} else {
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
	while(curr != NULL)
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
		//next 3 lines are to restore q->last
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

/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{	
	struct Node *tmp = (&q)->front;
	printf("\nScheduler: Printing tasks.\nCurrent task (id:%d, pid:%d, name:%s)\n",
			tmp->id,tmp->pid,tmp->name);
	//Sigoura tha typothei ena task, to shell
	tmp = tmp->next;
	if(tmp!=NULL)
	{
		printf("Rest of the tasks:\n");
		while (tmp != NULL) 
		{
			printf("(id:%d, pid:%d, name:%s)\n",tmp->id,tmp->pid,tmp->name);
			tmp = tmp->next;
		}
	}
		printf("End of tasks.\n\n");
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int
sched_kill_task_by_id(int id) //Prepei na brethei to pid
{
	int pid;
	pid=find_pid(&q,id); //Brisketai to pid apo to id
	struct Node * curr=find_node_from_pid(&q,pid); //Brisketai kai o antistoixos komvos
	if (curr!=NULL)
		printf("Scheduler: Killing (id:%d, pid:%d, name:%s).\n",
			curr->id,curr->pid,curr->name);
	else { 
		printf("Scheduler: Process not found.\n");
		return 0;
	}
	return kill(pid, SIGKILL);
}

/* Create a new task. */
static void
sched_create_task(char *executable)
{
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };
	pid_t p = fork();
	if(p < 0){
		perror("error: fork");
		exit(1);
	}
	else if(p == 0){
		raise(SIGSTOP); //child process raises SIGSTOP immediately
		execve(executable, newargv, newenviron);
		perror("execve");
		exit(1);
	}
	else{
		printf("Scheduler: Creating (id:%d, pid:%d, name:%s).\n",
			next_id(&q),p,executable);
		push(&q,next_id(&q),p,executable); //Scheduler pushes new child's pid to queue
		increase_next_id(&q); //auksise to epomeno id afou xrisimopoiisame ena gia ti nea diergasia
	}
}

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);
			return 0;

		default:
			return -ENOSYS;
	}
}

/*
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	// printf("alarm handler used\n");
	pid_t to_stop = (front(&q)->pid); 
	printf("Scheduler: Stopping (id:%d, pid:%d, name:%s).\n",
			front(&q)->id,front(&q)->pid,front(&q)->name);
	kill(to_stop, SIGSTOP);
}

/*
 * SIGCHLD handler
 */
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
		p = waitpid(-1, &status, WUNTRACED | WNOHANG);
		if (p < 0) {
			perror("waitpid");
			exit(1);
		}
		if (p == 0)
			break;

		explain_wait_status(p, status);

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
			//	if(empty(&q)){
			//		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
			//		exit(1);
			//	}
			//	return;
			//}
		}
		if (WIFSTOPPED(status)) {
			/* A child has stopped due to SIGSTOP/SIGTSTP, etc... */
			if(p==front(&q)->pid){
				push(&q,front(&q)->id,front(&q)->pid,front(&q)->name); 
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
		kill((front(&q)->pid), SIGCONT); //send SIGCONT to next process in queue
	}
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
		perror("signals_disable: sigprocmask");
		exit(1);
	}
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGALRM);
	sigaddset(&sigset, SIGCHLD);
	if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
		perror("signals_enable: sigprocmask");
		exit(1);
	}
}


/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
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

static void
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static void
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);
		assert(0);
	}
	/* Parent */
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];

	push(&q,next_id(&q),p,"shell"); //Scheduler adds shell to queue
	increase_next_id(&q);
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}

		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int nproc;
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;
	init(&q);
	/* Create the shell. */
	sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
	/* TODO: add the shell to the scheduler's tasks */
	//Done inside sched_create_shell

	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	 int i; //arithmos programmaton pou tha eisaxthoun sto scheduler
	 for(i = 1; i < argc; i++)
	 {
	 	char *executable = argv[i];
	 	char *newargv[] = { argv[i], NULL, NULL, NULL };
	 	char *newenviron[] = { NULL };
	 	pid_t p = fork();
	 	if(p < 0){
	 		perror("error: fork");
	 		exit(1);
	 	}
	 	else if(p == 0){
	 		raise(SIGSTOP); //child process raises SIGSTOP immediately
	 		execve(executable, newargv, newenviron);
	 		perror("execve");
	 		exit(1);
	 	}
	 	else{
	 		push(&q,next_id(&q),p,executable); //Scheduler pushes child to queue
			increase_next_id(&q); //auksanoume to epomeno id
	 	}
	 }
	 
	 nproc = argc; /* number of processes goes here */

	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);

	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}

	print_queue(&q);
	printf("All initialized correctly. Please proceed.\n");

	alarm(SCHED_TQ_SEC); //set an alarm for SCHED_TQ_SEC
	printf("Scheduler: Activating (id:%d, pid:%d, name:%s).\n",
			front(&q)->id,front(&q)->pid,front(&q)->name);
	kill((front(&q)->pid), SIGCONT); //send SIGCONT to first process
	shell_request_loop(request_fd, return_fd);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
