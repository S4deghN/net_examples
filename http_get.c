// TODO: 

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

#define shift(xs_sz, xs) ((xs_sz)--, *(xs)++)

typedef struct {
    char* data;
    size_t size;
    size_t capacity;
} Buffer;

#define BUFF_INIT_CAP 128

#define _append(buff, chunk, n) ({                        \
    if (buff->capacity == 0) {                            \
        buff->capacity = BUFF_INIT_CAP;                   \
        buff->data = malloc(buff->capacity);              \
    }                                                     \
    if (buff->size + n > buff->capacity) {                \
        while (buff->size + n > buff->capacity) {         \
            buff->capacity *= 2;                          \
        }                                                 \
        buff->data = realloc(buff->data, buff->capacity); \
    }                                                     \
                                                          \
    memcpy(buff->data + buff->size, chunk, n);            \
    buff->size += n;                                      \
})

void append(Buffer* buff, const char* chunk, size_t n) {
    _append(buff, chunk, n);
}

void append_cstr_va(Buffer* buff, ...) {
    va_list args;
    va_start(args, buff);
    const char* str;
    while ((str = va_arg(args, const char *))) {
        append(buff, str, strlen(str));
    }
    va_end(args);
}

void append_cstr(Buffer* buff, const char* str) {
    append(buff, str, strlen(str));
}

typedef struct {
    const char* data;
    size_t size;
} StringView;

typedef struct {
    const char* scheme;
    const char* domain;
    const char* port;
    const char* path;
} Url;

ssize_t find_sequence(const char* data, size_t data_size, const char* sequence) {
    size_t seq_size = strlen(sequence);
    for (size_t i = 0; i + seq_size - 1 < data_size; ++i) {
        if (strncmp(data + i, sequence, seq_size) == 0) {
            return i;
        }
    }
    return -1;
}

ssize_t find_char(const char* data, size_t data_size, const char* c) {
    for (size_t i = 0; i < data_size; ++i) {
        for (size_t j = 0; j < strlen(c); ++j) {
            if (data[i] == c[j]) {
                return i;
            }
        }
    }
    return -1;
}

char* split_by_seq(char** str, const char* seq) {
    ssize_t idx = find_sequence(*str, strlen(*str), seq);
    if (idx == -1) {
        return *str;
    }

    char* ret = *str;
    (*str)[idx] = '\0';
    *str += idx + strlen(seq);
    return ret;
}

char* split_by_char(char** str, const char* c) {
    char* ret = *str;
    ssize_t idx = find_char(*str, strlen(*str), c);

    if (idx == -1) {
        *str += strlen(*str);
        return ret;
    }

    (*str)[idx] = '\0';
    *str += idx + 1;
    return ret;
}

Url parse_url(char* url_str) {
    Url url = {0};

    if (find_sequence(url_str, strlen(url_str), "://") != -1) {
        url.scheme = split_by_seq(&url_str, "://");
    } else {
        url.scheme = "http";
    }

    if (find_char(url_str, strlen(url_str), ":") != -1) {
        url.domain = split_by_char(&url_str, ":");
        url.port = split_by_char(&url_str, "/");
    } else {
        url.domain = split_by_char(&url_str, "/");
        url.port = NULL;
    }

    if (strlen(url_str) > 0) {
        url.path = url_str;
    } else {
        url.path = "/";
    }

    return url;
}

void print_addr(struct addrinfo* addr) {
    char buff[INET_ADDRSTRLEN];
    struct sockaddr_in* addr_in = (void*)addr->ai_addr;
    inet_ntop(AF_INET, &addr_in->sin_addr, buff, sizeof(buff));
    fprintf(stderr, "addr_info: IP = %s\n", buff);
    fprintf(stderr, "addr_info: port = %d\n", ntohs(addr_in->sin_port));
}

int main(int argc, char* argv[]) {
    char* program_name = shift(argc, argv);
    if (argc < 1) {
        fprintf(stderr, "Usage: %s <url>\n", program_name);
        exit(1);
    }
    char* url_str = shift(argc, argv);

    Url url = parse_url(url_str);
    fprintf(stderr, "url.scheme = %s\n", url.scheme);
    fprintf(stderr, "url.domain = %s\n", url.domain);
    fprintf(stderr, "url.port = %s\n", url.port);
    fprintf(stderr, "url.path = %s\n", url.path);

    struct addrinfo hints = {0};
    struct addrinfo* addrs;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int ret = getaddrinfo(url.domain, url.port?:url.scheme, &hints, &addrs);
    if (ret != 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(ret));
        perror("");
        exit(1);
    }

    int sfd;
    for (struct addrinfo* addr = addrs; addr != NULL; addr = addr->ai_next) {
        print_addr(addr);

        sfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sfd == -1) break;

        // NOTE: `addrlen` basically indicates the type of the sockaddr structure.
        if (connect(sfd, addr->ai_addr, addr->ai_addrlen) == 0) break;

        close(sfd);
        sfd = -1;
    } if (sfd == -1) {
        fprintf(stderr, "Error: Could not connect!\n");
        perror("");
        exit(1);
    }
    freeaddrinfo(addrs);

    Buffer get_request = {0};
    append_cstr_va(&get_request, "GET ", url.path, " HTTP/1.1\r\n",
                                 "Host: ", url.domain, "\r\n",
                                 "Connection: Close\r\n",
                                 "\r\n", NULL);

    ssize_t n = send(sfd, get_request.data, get_request.size, 0);
    fprintf(stderr, "   %zu bytes sent:\n%.*s\n", n, (int)get_request.size, get_request.data);

    Buffer buff = {0};

    while(1) {
        char read_buff[1024];
        int n = recv(sfd, read_buff, sizeof(read_buff), 0);
        if (n <= 0) {
            fprintf(stderr, "peer diconnected!\n");
            break;
        }
        append(&buff, read_buff, n);
    }

    int http_payload_inx = find_sequence(buff.data, buff.size, "\r\n\r\n");
    http_payload_inx += 4;
    fprintf(stderr, "   HEADER:\n%.*s", http_payload_inx, buff.data);
    write(STDOUT_FILENO, buff.data + http_payload_inx, buff.size - http_payload_inx);
}
