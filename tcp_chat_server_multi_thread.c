// TODO:
// * [x] make the msg_queue an actual queue. right now it is a stack and in
// case of long messages they get broken down in wrong order.

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <semaphore.h>

#define IP(a, b, c, d) (uint32_t)((a) << 24 | (b) << 16 | (c) << 8 | (d))
#define countof(a) sizeof(a)/sizeof(a[0])
#define sv_fmt(sv) (int)(sv).size, (char*)(sv).data

struct fd_list {
    int data[128];
    int count;
    pthread_mutex_t mutex;
};

void fd_remove(struct fd_list* list, int item) {
    pthread_mutex_lock(&list->mutex);
    for (int i = 0; i < countof(list->data); ++i) {
        if (list->data[i] == item) {
            list->data[i] = list->data[list->count - 1];
            list->count--;
        }
    }
    pthread_mutex_unlock(&list->mutex);
}

void fd_add(struct fd_list* list, int item) {
    pthread_mutex_lock(&list->mutex);
    if (list->count < countof(list->data)) {
        list->data[list->count] = item;
        list->count++;
    } else {
        assert(0 && "fd_list is full");
    }
    pthread_mutex_unlock(&list->mutex);
}

struct msg {
    char data[128];
    int size;
};

// must be power of 2 for the queue implementation to work!
constexpr int Q_size = 1 << 7;
constexpr uint Q_mask = Q_size - 1;
struct msg_queue {
    struct msg data[Q_size];
    uint head;
    uint tail;
    sem_t can_send;
    sem_t can_enqueue;
    pthread_mutex_t mutex;
};

void init_msg_queue(struct msg_queue* q) {
    q->head = 0;
    q->tail = 0;
    assert(sem_init(&q->can_send, 0, 0) == 0);
    assert(sem_init(&q->can_enqueue, 0, countof(q->data)) == 0);
    assert(pthread_mutex_init(&q->mutex, NULL) == 0);
}

struct msg_queue msg_queue;
struct fd_list fd_list;

void* client_h(void* args) {
    int cfd = *(int*)args;
    printf("created client handle for fd=%d\n", cfd);
    struct msg msg;

    while(1) {
        msg.size = recv(cfd, msg.data, sizeof(msg.data), 0);
        if (msg.size <= 0) {
            break;
        }

        sem_wait(&msg_queue.can_enqueue);

        pthread_mutex_lock(&msg_queue.mutex);
        uint index = (msg_queue.head++) & Q_mask;
        memcpy(msg_queue.data[index].data, msg.data, msg.size);
        msg_queue.data[index].size = msg.size;
        pthread_mutex_unlock(&msg_queue.mutex);

        sem_post(&msg_queue.can_send);
    }

    if (msg.size == -1) {
        printf("client:%d: error: %s!\n", cfd, strerror(errno));
    }
    assert(shutdown(cfd, SHUT_WR) != -1);
    while(recv(cfd, msg.data, sizeof(msg.data), 0) > 0);
    assert(close(cfd) != -1);
    fd_remove(&fd_list, cfd);
    printf("client:%d: closed!\n", cfd);
    return NULL;
}

void* dispatcher_h(void* args) {
    while(1) {
        sem_wait(&msg_queue.can_send);

        int index = msg_queue.tail++ & Q_mask;
        void* data = msg_queue.data[index].data;
        size_t size = msg_queue.data[index].size;
        for (int i = 0; i < fd_list.count; ++i) {
            // printf("client:%d: sending: '%.*s'\n", fd_list.data[i], (int)size, (char*)data);
            int n = send(fd_list.data[i], data, size, 0);
            if (n <= 0) {
                printf("client:%d: error: %s!\n", fd_list.data[i], strerror(errno));
            }
        }

        sem_post(&msg_queue.can_enqueue);
    }

    return NULL;
}

int main() {
    // initialize msg_queue
    init_msg_queue(&msg_queue);

    int ret;
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(sfd != -1);

    int one = 1;
    assert(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) != -1);

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

    pthread_t dispatcher;
    pthread_create(&dispatcher, NULL, dispatcher_h, NULL);

    while(1) {
        int cfd = accept(sfd, NULL, NULL);
        assert(cfd != -1);

        fd_add(&fd_list, cfd);

        pthread_t tr;
        pthread_create(&tr, NULL, client_h, &fd_list.data[fd_list.count - 1]);
    }

}
