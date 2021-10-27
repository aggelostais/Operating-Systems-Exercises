#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

int computation(int a, int b, char op)
{
    if(op == '+') return a + b;
    else if(op == '*') return a * b;
}

void fork_procs(struct tree_node *ptr, int pipe_to_parent)
{
        /*
         * Start
         */
        printf("PID = %ld, name %s, starting...\n",
                        (long)getpid(), ptr->name);
        change_pname(ptr->name);
        
        if((ptr->nr_children)==0)
        {
                //to fyllo koimatai
                raise(SIGSTOP);

                //to fyllo jypna
                
                int res = atoi(ptr->name);

                if (write(pipe_to_parent, &res, sizeof(res)) != sizeof(res)){
                    perror("leaf: write to pipe");
                    exit(1);
                }

                printf("%s: Exiting...\n",ptr->name);
                exit(res);
        }

        //Periptosi opou exei paidia
        int pipe_to_child[2];
        if (pipe(pipe_to_child) < 0) {
                perror("pipe");
                exit(1);
        }

        pid_t p[(ptr->nr_children)+1]; //Pinakas pou krataei ta pid olon ton paidion tou ekastote komvou
        int status;
        
        p[1]=fork();
        if(p[1] < 0)
        {
            perror("fork");
            exit(0);
        }
        else if(p[1] == 0)
            fork_procs(ptr->children, pipe_to_child[1]); //children take pipe[1] to write
        
        p[2]=fork();
        if(p[2] < 0)
        {
            perror("fork");
            exit(0);
        }
        else if(p[2] == 0)
            fork_procs(ptr->children+1, pipe_to_child[1]); //children take pipe[1] to write

        wait_for_ready_children(ptr->nr_children);
        /*
         * Suspend Self
         */
        raise(SIGSTOP);

        printf("PID = %ld, name = %s is awake\n",
                (long)getpid(), ptr->name);

        //parent sets children in motion one-by-one
        int a, b;

        //child 1
        kill(p[1], SIGCONT);
        wait(&status);
        explain_wait_status(p[1], status);
        if (read(pipe_to_child[0], &a, sizeof(a)) != sizeof(a)) {
                perror("parent: read from pipe");
                exit(1);
        }

        //child 2
        kill(p[2], SIGCONT);
        wait(&status);
        explain_wait_status(p[2], status);
        if (read(pipe_to_child[0], &b, sizeof(b)) != sizeof(b)) {
                perror("parent: read from pipe");
                exit(1);
        }

        int ret = computation(a, b, *(ptr->name));

        if (write(pipe_to_parent, &ret, sizeof(ret)) != sizeof(ret)) {
                perror("node: write to pipe");
                exit(1);
        }

        

        printf("%s: Exiting...\n",ptr->name);
        /*
         * Exit
         */
        exit(ret);
}

int main(int argc, char *argv[])
{
        pid_t pid;
        int status;
        struct tree_node *root;

        if (argc < 2){
                fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
                exit(1);
        }

        /* Read tree into memory */
        root = get_tree_from_file(argv[1]);

        int pipe_root[2];

        printf("root: Creating pipe...\n");
        if (pipe(pipe_root) < 0) { //an apotyxei i dimiourgia tou pipe minima sfalmatos
                perror("pipe");
                exit(1);
        }

        /* Fork root of process tree */
        pid = fork();
        if (pid < 0) {
                perror("main: fork");
                exit(1);
        }
        if (pid == 0) {
                /* Child */
                fork_procs(root, pipe_root[1]);
                exit(1);
        }
        /* for ask2-signals */
        wait_for_ready_children(1);

        /* Print the process tree root at pid */
        show_pstree(pid);

        /* for ask2-signals */
        kill(pid, SIGCONT);

        /* Wait for the root of the process tree to terminate */
        wait(&status);
        explain_wait_status(pid, status);

        int final;
        if (read(pipe_root[0], &final, sizeof(final)) != sizeof(final)) {
                perror("root: read from pipe");
                exit(1);
        }
        printf("final result = %d\n", final);

        return 0;
}


