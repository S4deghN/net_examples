#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#define IP(a, b, c, d) (uint32_t)((a) << 24 | (b) << 16 | (c) << 8 | (d))

int main() {
    int ret;
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("fd = %d\n", fd);

    struct sockaddr_in addr = {
        AF_INET,
        htons(5555),
        {htonl(IP(127,0,0,1))}
    };
    ret = bind(fd, (void*)&addr, sizeof(addr));
    printf("bind ret = %d\n", ret);
    if (ret != 0) {
        printf("errno: %d, %s\n", errno, strerror(errno));
        return 1;
    }

    ret = listen(fd, 1);
    if (ret != 0) {
        printf("errno: %d, %s\n", errno, strerror(errno));
        return 1;
    }

    while(1) {
        printf("Listening\n");
        int cfd = accept(fd, NULL, NULL);
        printf("cfd = %d\n", cfd);
        if (cfd == -1) {
            printf("errno: %d, %s\n", errno, strerror(errno));
            return 1;
        }

        while(1) {
            char buff[32] = {0};
            ssize_t n = recv(cfd, buff, sizeof(buff), 0);
            if (n == 0) { // indicates closed connection
                printf("Connection closed!\n");
                break;
            }
            printf("received %1$d bytes: %2$*1$s\n", (int)n, buff);

            n = send(cfd, buff, n, 0);
            printf("sent %ld bytes!\n", n);
        }
    }
}
