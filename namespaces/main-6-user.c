/* Copyright (C) 2014 Leslie Zhai <xiang.zhai@i-soft.com.cn> */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define STACK_SIZE (1024 * 1024)
 
// sync primitive
int checkpoint[2];
 
static char child_stack[STACK_SIZE];
char* const child_args[] = {
    "/bin/bash", 
    NULL
};
 
int child_main(void* arg)
{
    char c;
    
    // init sync primitive 
    close(checkpoint[1]);
    // wait...
    read(checkpoint[0], &c, 1);
 
    printf(" - [%5d] [%5d] World !\n", getpid(), getuid());
    sethostname("LeslieContainer", 15);
    execv(child_args[0], child_args);
    printf("Ooops\n");
    return 1;
}
 
int main(int argc, char* argv[])
{
    // init sync primitive
    pipe(checkpoint);
 
    printf(" - [%5d] [%5d] Hello ?\n", getpid(), getuid());
 
    int child_pid = clone(child_main, child_stack + STACK_SIZE, 
                          CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWPID | 
                          CLONE_NEWUSER | SIGCHLD, 
                          NULL);
    if (child_pid == -1) {
        printf("ERROR: fail to clone %s\n", strerror(errno));
    }
 
    // further init here (nothing yet)
 
    // signal "done"
    close(checkpoint[1]);
 
    waitpid(child_pid, NULL, 0);
    return 0;
}
