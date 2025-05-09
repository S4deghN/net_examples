#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#define IPv4(a, b, c, d) (uint32_t)((a) << 24 | (b) << 16 | (c) << 8 | (d))

int main() {
    int ret;

    int sfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (sfd == -1) {
        printf("error: %d, %s\n", errno, strerror(errno));
    }


    struct sockaddr_in addr = {
        AF_INET,
        htons(5555),
        {htonl(IPv4(127,0,0,1))}
    };
    ret = bind(sfd, (void*)&addr, sizeof(addr));
    if (ret != 0) {
        printf("error: %d, %s\n", errno, strerror(errno));
    }


    while(1) {
        int n;
        char buff[128];
        struct sockaddr_in c_addr = {0};
        unsigned int c_addr_len = sizeof(c_addr); // This is an input and output argument but in case of inet, the size of addresses are fixed and we can ignore what is returned. At least that's what I believe.
        n = recvfrom(sfd, buff, sizeof(buff), 0, (void*)&c_addr, &c_addr_len);
        if (n == -1) {
            printf("error: %d, %s\n", errno, strerror(errno));
            break;
        }
        printf("received %d bytes from %x:%d: %*s\n", n,
            ntohl(c_addr.sin_addr.s_addr),
            ntohs(c_addr.sin_port), n, buff);

        n = sendto(sfd, buff, n, 0, (void*)&c_addr, c_addr_len);
        if (n == -1) {
            printf("error: %d, %s\n", errno, strerror(errno));
            break;
        }
    }
}
