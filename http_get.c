#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <netdb.h>
#include <string.h>

// TODO: log the resolved IP.

int main() {
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sfd != -1);

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* result;
    struct addrinfo* next_res;
    int ret = getaddrinfo("google.com", "80", &hints, &result);
    if (ret != 0) {
        printf("Error: %s\n", gai_strerror(ret));
    }
    for (next_res = result; next_res != NULL; next_res = next_res->ai_next) {
        // NOTE: `addrlen` basically indicates the type of the sockaddr structure.
        if (next_res->ai_family != AF_INET || next_res->ai_socktype != SOCK_STREAM) {
            continue;
        }
        if (connect(sfd, next_res->ai_addr, next_res->ai_addrlen) != -1) {
            break;
        }
    } if (next_res == NULL) {
        printf("Error: Could not connect!\n");
        return 1;
    }

    freeaddrinfo(result);

    const char* get_request = "GET / HTTP/1.1\r\n"
                              "\r\n";
    ssize_t n = send(sfd, get_request, strlen(get_request), 0);
    printf("%zu bytes sent!\n", n);

    while(1) {
        char buff[128];
        int n = recv(sfd, buff, sizeof(buff), 0);
        if (n <= 0) {
            printf("peer diconnected!\n");
            break;
        }
        printf("%.*s", n, buff);
    }
}
