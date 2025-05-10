#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#define IPv4(a, b, c, d) (uint32_t)((a) << 24 | (b) << 16 | (c) << 8 | (d))

// TODO: read signal(7) page.
void sigchld_h(int sig) {
    waitpid(-1, NULL, WNOHANG);
    // printf("waitid() = %d!\n", ret);
}

int main() {
    if (signal(SIGCHLD, sigchld_h) == SIG_ERR) {
        printf("error:%d: %s!\n", errno, strerror(errno));
        return 1;
    }

    int ret;
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sfd > 0);

    struct sockaddr_in s_addr = {
        AF_INET,
        htons(5555),
        {htonl(IPv4(127,0,0,1))}
    };
    uint s_addr_len = sizeof(s_addr);
    ret = bind(sfd, (void*)&s_addr, s_addr_len);
    assert(ret == 0);

    ret = listen(sfd, 128);

    while(1) {
        // TODO: read man and see what happens to fd on fork.
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
                ret = shutdown(cfd, SHUT_RDWR);
                printf("client:%d: closed!\n", cfd);

                return 0;
            }

            n = send(cfd, buff, sizeof(buff), 0);
        }
    }
}
