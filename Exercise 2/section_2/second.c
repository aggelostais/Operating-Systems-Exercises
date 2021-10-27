#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "proc-common.h"
#include "tree.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3
typedef struct tree_node * tree_node_ptr;

void fork_process(tree_node_ptr ptr)
{
    printf("%s: Created...\n",ptr->name);
    change_pname(ptr->name); //Allazei to onoma tis diergasias
    if((ptr->nr_children)==0)
    {
        printf("%s: Sleeping...\n",ptr->name);
        sleep(SLEEP_PROC_SEC);
        printf("%s: Exiting...\n",ptr->name);
        exit(0);
    }
    //Periptosi opou exei paidia
    pid_t p[(ptr->nr_children)+1]; //Pinakas pou krataei ta pid olon ton paidion tou ekastote komvou
    int i, status;
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
    printf("%s: Waiting for children to terminate...\n",ptr->name);
    for(i=1;i<=ptr->nr_children;i++)
        {
            p[i] = wait(&status);
            explain_wait_status(p[i], status);
        }
    printf("%s: Exiting...\n",ptr->name);
    exit(0);
}

int main(int argc, char *argv[])
{
    struct tree_node *root;
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
        exit(1);
    }
    root = get_tree_from_file(argv[1]); //Pairnei diekti sto root tou dentrou
    print_tree(root); //Typonoume to dentro gia na doume an to bgazei sosta sto telos

    pid_t pid;
    int status;
    if (root==NULL)
    {
        printf("Tree is empty. Exiting...\n");
        return(0);
    }
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
    /*Father */
    sleep(SLEEP_TREE_SEC);
    /* Print the process tree root at pid */
    show_pstree(pid);
    /* Wait for the root of the process tree to terminate */
    pid = wait(&status);
    explain_wait_status(pid, status);
    return 0;
}
