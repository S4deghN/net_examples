#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#define IPv4(a, b, c, d) (uint32_t)((a) << 24 | (b) << 16 | (c) << 8 | (d))

int main() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        printf("error:%d: %s\n", errno, strerror(errno));
    }

    struct sockaddr_in s_addr = {
        AF_INET,
        htons(5555),
        {htonl(IPv4(127,0,0,1))},
    };
    unsigned int s_addr_len = sizeof(s_addr);

    while(1) {
        int n;
        char buff[128];
        fgets(buff, sizeof(buff), stdin);

        n = sendto(fd, buff, strlen(buff), 0, (void*)&s_addr, s_addr_len);
        if (n == -1) {
            printf("error:%d: %s\n", errno, strerror(errno));
            break;
        }
        printf("sent %d bytes\n", n);

        n = recvfrom(fd, buff, sizeof(buff), 0, (void*)&s_addr, &s_addr_len);
        if (n == -1) {
            printf("error:%d: %s\n", errno, strerror(errno));
            break;
        }
        printf("received %d bytes: %.*s\n", n, n, buff);

    }
}
