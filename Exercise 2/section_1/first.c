#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */

void child_D(void){
        change_pname("D"); //Ylopoimeni synartisi pou allazei to onoma tis diergasias
        printf("D starting\n");
        printf("D: Sleeping...\n");
        sleep(SLEEP_PROC_SEC);

        printf("D: Exiting...\n");
        exit(13);
}


void child_C(void){
        change_pname("C");
        printf("C starting\n");
        printf("C: Sleeping...\n");
        sleep(SLEEP_PROC_SEC);

        printf("C: Exiting...\n");
        exit(17);
}

void child_B(void){
        printf("B starting\n");
        change_pname("B");

        int statusD;
        pid_t pD = fork();
        if(pD < 0){
                perror("B: fork");
                exit(1);
        }
        else if(pD == 0){
            child_D();
        }

        printf("B waiting for children to terminate\n");

        pD = wait(&statusD);
        explain_wait_status(pD, statusD);

        printf("B: Exiting...\n");
        exit(19);
}

void fork_procs(void)
{
        /*
         * initial process is A.
         */
        printf("A starting\n");
        change_pname("A");

        int statusB, statusC;
        pid_t pB, pC;

        pC = fork();
        if(pC < 0){
                perror("A: fork");
                exit(1);
        }
        //To paidi C bainei se auto to tmima
        else if(pC == 0){
            child_C();
        }

        pB = fork();
        if(pB < 0){
                perror("A: fork");
                exit(1);
        }
        else if(pB == 0){
            child_B();
        }

        printf("A waiting for children to terminate\n");
        //Perimenei na termatisoun me ti seira ta paidia B kai C
        pB = wait(&statusB);
        pC = wait(&statusC);
        explain_wait_status(pB, statusB);
        explain_wait_status(pC, statusC);

        printf("A: Exiting...\n");
        exit(16); //Kanei exit me to katallilo exit status pou tha syllexthei apo th wait
}
/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */
int main(void)
{
        pid_t pid;
        int status;

        /* Fork root of process tree */
        pid = fork();
        //Fork failed.
        if (pid < 0) {
                perror("main: fork");
                exit(1);
        }
        if (pid == 0) {
                /* Child */
                fork_procs();
                exit(1);
        }

        sleep(SLEEP_TREE_SEC);

        /* Print the process tree root at pid */
        show_pstree(pid);

        /* Wait for the root of the process tree to terminate */
        pid = wait(&status);
        explain_wait_status(pid, status); //emvanizei to katallilo minima gia
                                          //to logo allagis katastasis ton paidion tou

        return 0;
}
