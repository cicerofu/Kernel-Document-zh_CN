#include <stdio.h>         /* printf */
#include <sys/prctl.h>     /* prctl */
#include <linux/seccomp.h> /* seccomp's constants */
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>        /* dup2: just for test */

#define BUF_SIZE 1024

int main(int argc, char* argv[]) 
{
    int fd = -1;
    char buf[BUF_SIZE] = {'\0'};
    printf("step 1: unrestricted\n");

    // Enable filtering
    prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT);
    printf("step 2: only 'read', 'write', '_exit' and 'sigreturn' syscalls\n");

    fd = open("/dev/input/event16", O_RDONLY | O_NONBLOCK);
    if (fd != -1) {
        read(fd, buf, BUF_SIZE);
        printf("DEBUG: %s\n", buf);
    } else {
        printf("ERROR: %s\n", strerror(errno));
    }

    // Redirect stderr to stdout
    dup2(1, 2);
    printf("step 3: !! YOU SHOULD NOT SEE ME !!\n");

    // Success (well, not so in this case...)
    return 0; 
}
