#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define IPv4(a, b, c, d) (uint32_t)((a) << 24 | (b) << 16 | (c) << 8 | (d))

#define SIGCHLD_RT SIGRTMIN+1

// TODO: read signal(7) page.
// only _SC_SIGQUEUE_MAX amount of queued real-time signals are permitted (at
// least that's what I understand form documentations) but this just works fine
// and we don't get any zombies.
void sigchld_h(int sig) {
    waitpid(-1, NULL, 0);
    // printf("waitid() = %d!\n", ret);
}

int main() {
    // ignoring the SIGCHLD let's any child that terminates exit and be waited
    // upon without any work required.
    signal(SIGCHLD, SIG_IGN);

    // if (signal(SIGCHLD_RT, sigchld_h) == SIG_ERR) {
    //     printf("error:%d: %s!\n", errno, strerror(errno));
    //     return 1;
    // }

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sfd > 0);

    struct sockaddr_in s_addr = {
        AF_INET,
        htons(5555),
        {htonl(IPv4(127,0,0,1))}
    };
    uint s_addr_len = sizeof(s_addr);
    assert(bind(sfd, (void*)&s_addr, s_addr_len) == 0);

    assert(listen(sfd, 128) == 0);

    while(1) {
        // TODO: read man pages to find out what exactly happens to cfd and how
        // to controll it.
        int cfd = accept(sfd, NULL, NULL);

        pid_t pid = fork();
        while(pid == 0) {
            int n;
            char buff[128];

            n = recv(cfd, buff, sizeof(buff), 0);
            if (n <= 0) {
                if (n == -1) {
                    printf("client:%d: error: %s!\n", cfd, strerror(errno));
                }
                assert(shutdown(cfd, SHUT_RDWR) != -1);
                assert(close(cfd) != -1);
                printf("client:%d: closed!\n", cfd);

                // int ret = sigqueue(getppid(), SIGCHLD_RT, (union sigval){0});
                // assert(ret != -1);
                exit(0);
            }

            n = send(cfd, buff, n, 0);
        }
    }
}
