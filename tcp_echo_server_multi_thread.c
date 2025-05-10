#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#define IP(a, b, c, d) (uint32_t)((a) << 24 | (b) << 16 | (c) << 8 | (d))

void* echo_thread_h(void* args) {
    int cfd = *(int*)args;
    while(1) {
        char buff[128] = {0};
        ssize_t n = recv(cfd, buff, sizeof(buff), 0);
        if (n <= 0) {
            if (n == -1) {
                printf("client:%d: error: %s!\n", cfd, strerror(errno));
            }
            assert(shutdown(cfd, SHUT_RDWR) != -1);
            assert(close(cfd) != -1);
            printf("client:%d: closed!\n", cfd);
            return NULL;
        }
        n = send(cfd, buff, n, 0);
    }
}


int main() {
    int ret;
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sfd != -1);

    struct sockaddr_in s_addr = {
        AF_INET,
        htons(5555),
        {htonl(IP(127,0,0,1))}
    };
    ret = bind(sfd, (void*)&s_addr, sizeof(s_addr));
    if (ret != 0) {
        printf("errno: %d, %s\n", errno, strerror(errno));
        return 1;
    }

    assert(listen(sfd, 128) == 0);

    while(1) {
        int cfd = accept(sfd, NULL, NULL);
        assert(cfd != -1);

        pthread_t tr;
        pthread_create(&tr, NULL, echo_thread_h, &cfd);
    }

}
