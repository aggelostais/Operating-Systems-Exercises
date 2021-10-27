#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "proc-common.h"
#include "tree.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3
typedef struct tree_node * tree_node_ptr;

void fork_process(tree_node_ptr ptr)
{
	printf("PID = %ld, name %s, starting...\n",
              	(long)getpid(), ptr->name);
  	change_pname(ptr->name); //Allazei to onoma tis diergasias
  	if((ptr->nr_children)==0)
	{
  	  	raise(SIGSTOP); //Anastellei ti leitourgia tis
  	  	printf("PID = %ld, name = %s is awake\n",(long)getpid(),ptr->name);
	    	printf("PID = %ld, name = %s exiting\n",(long)getpid(),ptr->name);
	    	exit(0);
  	}
	//Periptosi opou exei paidia
	pid_t p[(ptr->nr_children)+1]; //Pinakas pid olon ton paidion tou komvou
	int i, status;
	//Dimiourgia paidion
	for(i=1;i<=ptr->nr_children;i++)
    	{
        	p[i]=fork();
        	if(p[i] < 0)
        	{
            	perror("fork");
            	exit(0);
        	}
        	else if(p[i] == 0)
                fork_process(ptr->children+i-1);
    	}
    wait_for_ready_children(ptr->nr_children); //Perimenei na anastiloun ti
                                               //leitourgia tous ola ta paidia
  	raise(SIGSTOP); //Anastellei ti leitourgia tis

  	printf("PID = %ld, name = %s is awake\n",(long)getpid(),ptr->name);
      for(i=1;i<=ptr->nr_children;i++)
	{
        kill(p[i], SIGCONT); //Stelnei sima epanekkinisis sto kathe paidi
    	p[i] = wait(&status); //Perimenei na termatisei
    	explain_wait_status(p[i], status); //Typonei minima gia to logo termatismou
	}
  	printf("%s: Exiting...\n",ptr->name);
  	exit(0);
}

int main(int argc, char *argv[])
{
  	pid_t pid;
  	int status;
  	struct tree_node *root;
  	if (argc < 2)
	  {
        	fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
        	exit(1);
  	}
  	/* Read tree into memory */
  	root = get_tree_from_file(argv[1]);
  	/* Fork root of process tree */
  	pid = fork();
  	if (pid < 0)
	  {
        	perror("main: fork");
        	exit(1);
  	}
  	if (pid == 0)
	  {
        	/* Child */
        	fork_process(root);
        	exit(1);
  	}
  	wait_for_ready_children(1);
  	/* Print the process tree root at pid */
  	show_pstree(pid);
  	/* for ask2-signals */
  	kill(pid, SIGCONT);
  	/* Wait for the root of the process tree to terminate */
  	wait(&status);
  	explain_wait_status(pid, status);
  	return 0;
}
