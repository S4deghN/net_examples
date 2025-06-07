#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#define __unused __attribute__((__unused__))
#define IP(a, b, c, d) (uint32_t)((a) << 24 | (b) << 16 | (c) << 8 | (d))

void*
stdin_h(void* args) {
    int fd = *(int*)args;

    while(1) {
        char buff[128];
        fgets(buff, sizeof(buff), stdin);

        __unused int n = send(fd, buff, strlen(buff), 0);
        // printf("sent %d bytes\n", n);
    }
    return NULL;
}

int main() {
    int ret;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd != 0);

    struct sockaddr_in addr = {
        AF_INET,
        htons(5555),
        {htonl(IP(127,0,0,1))}
    };
    ret = connect(fd, (void*)&addr, sizeof(addr));
    if (ret != 0) {
        printf("errno: %d, %s\n", errno, strerror(errno));
        return 1;
    }

    pthread_t stdin_tr;
    ret = pthread_create(&stdin_tr, NULL, &stdin_h, &fd);
    if (ret != 0) {
        printf("errno: %d, %s\n", errno, strerror(errno));
        assert(ret!= 0);
    }

    while(1) {
        char buff[128];
        int n = recv(fd, buff, sizeof(buff), 0);
        if (n <= 0) {
            printf("peer diconnected!\n");
            break;
        }
        printf("%.*s", n, buff);
        // printf("received %1$d bytes: %2$*1$s\n", n, buff);
    }

    ret = pthread_cancel(stdin_tr);
    assert(ret == 0);
}
