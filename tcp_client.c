#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#define IP(a, b, c, d) (uint32_t)((a) << 24 | (b) << 16 | (c) << 8 | (d))

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
    }

    while(1) {
        int n;
        char buff[128];
        fgets(buff, sizeof(buff), stdin);

        n = send(fd, buff, strlen(buff), 0);
        printf("sent %d bytes\n", n);

        n = recv(fd, buff, n, 0);
        printf("received %1$d bytes: %2$*1$s\n", (int)n, buff);
    }
}
