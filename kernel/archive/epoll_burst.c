#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/uio.h>        // recvmmsg
#include <pthread.h>

#define MAX_STUN       150
#define BATCH_SIZE     64
#define MAGIC_COOKIE   0x2112A442
#define STUN_PKT_LEN   20
#define TIME_LIMIT_MS  100

typedef struct {
    char host[256];
    uint16_t port;
    struct sockaddr_in addr;
    uint8_t transaction_id[12];
    struct timespec send_time;
    long rtt_ms;
} stun_ctx_t;

static stun_ctx_t stun_list[MAX_STUN];
static int stun_count = 0;
static int sock_fd;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void load_stun_servers() {
    const char *servers[] = {
        "stun.voipbuster.com", "stun.voipstunt.com", "stun.voipzoom.com",
        "stun.linphone.org",  "stun.antisip.com",   "stun.gmx.net",
        "stun.sipnet.net",    "stun.12voip.com",    "stun.3cx.com",
        "stun.ekiga.net"
        // ... extend up to MAX_STUN
    };
    int total = sizeof(servers)/sizeof(servers[0]);
    for(int i=0;i<total && stun_count<MAX_STUN;i++){
        struct addrinfo hints={0}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        if(getaddrinfo(servers[i], "3478", &hints, &res)==0) {
            strncpy(stun_list[stun_count].host, servers[i], sizeof(stun_list[0].host)-1);
            stun_list[stun_count].port = 3478;
            memcpy(&stun_list[stun_count].addr, res->ai_addr, sizeof(struct sockaddr_in));
            stun_list[stun_count].rtt_ms = -1;
            freeaddrinfo(res);
            stun_count++;
        }
    }
}

void *sender_thread(void *arg) {
    int idx = *(int*)arg;
    free(arg);
    // build STUN request
    uint8_t pkt[STUN_PKT_LEN] = {0};
    pkt[0] = 0x00; pkt[1] = 0x01; // Binding Request
    uint32_t cookie = htonl(MAGIC_COOKIE);
    memcpy(pkt+4, &cookie, 4);
    for(int j=0;j<12;j++){
        stun_list[idx].transaction_id[j] = rand() & 0xFF;
        pkt[8+j] = stun_list[idx].transaction_id[j];
    }
    clock_gettime(CLOCK_MONOTONIC, &stun_list[idx].send_time);
    sendto(sock_fd, pkt, STUN_PKT_LEN, 0,
           (struct sockaddr*)&stun_list[idx].addr, sizeof(struct sockaddr_in));
    return NULL;
}

void listener_loop() {
    struct mmsghdr msgs[BATCH_SIZE];
    struct iovec iovecs[BATCH_SIZE];
    uint8_t bufs[BATCH_SIZE][512];
    struct sockaddr_in froms[BATCH_SIZE];
    struct timespec now;
    for(int i=0;i<BATCH_SIZE;i++){
        iovecs[i].iov_base = bufs[i];
        iovecs[i].iov_len  = sizeof(bufs[i]);
        msgs[i].msg_hdr.msg_iov = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
        msgs[i].msg_hdr.msg_name = &froms[i];
        msgs[i].msg_hdr.msg_namelen = sizeof(froms[i]);
        msgs[i].msg_hdr.msg_control = NULL;
        msgs[i].msg_hdr.msg_controllen = 0;
        msgs[i].msg_len = 0;
    }

    int epfd = epoll_create1(0);
    struct epoll_event ev={0}; ev.events=EPOLLIN; ev.data.fd=sock_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int timeout = TIME_LIMIT_MS;
    while(1) {
        int nfds = epoll_wait(epfd, &ev, 1, timeout);
        if(nfds > 0) {
            int got = recvmmsg(sock_fd, msgs, BATCH_SIZE, 0, NULL);
            clock_gettime(CLOCK_MONOTONIC, &now);
            for(int m=0;m<got;m++){
                if(msgs[m].msg_len < STUN_PKT_LEN) continue;
                uint8_t *tid = bufs[m] + 8;
                for(int k=0;k<stun_count;k++){
                    if(stun_list[k].rtt_ms != -1) continue;
                    if(memcmp(tid, stun_list[k].transaction_id,12)==0){
                        long ms = (now.tv_sec - stun_list[k].send_time.tv_sec)*1000 +
                                   (now.tv_nsec - stun_list[k].send_time.tv_nsec)/1000000;
                        pthread_mutex_lock(&lock);
                        stun_list[k].rtt_ms = ms;
                        pthread_mutex_unlock(&lock);
                        break;
                    }
                }
            }
        }
        // check elapsed
        struct timespec cur; clock_gettime(CLOCK_MONOTONIC, &cur);
        long elapsed = (cur.tv_sec - start.tv_sec)*1000 + (cur.tv_nsec - start.tv_nsec)/1000000;
        if(elapsed >= TIME_LIMIT_MS) break;
        timeout = TIME_LIMIT_MS - elapsed;
    }
    close(epfd);
}

int main(){
    srand(time(NULL));
    load_stun_servers();

    sock_fd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);
    if(sock_fd < 0){ perror("socket"); return 1; }
    struct sockaddr_in local={0};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = 0;
    bind(sock_fd, (struct sockaddr*)&local, sizeof(local));

    // Spawn one thread per server
    pthread_t *senders = malloc(sizeof(pthread_t) * stun_count);
    for(int i=0;i<stun_count;i++){
        int *arg = malloc(sizeof(int)); *arg = i;
        pthread_create(&senders[i], NULL, sender_thread, arg);
    }
    for(int i=0;i<stun_count;i++) pthread_join(senders[i], NULL);
    free(senders);

    listener_loop();
    close(sock_fd);

    // Print fastest 5
    printf("\n🏁 En hızlı STUN sunucuları (<%d ms):\n", TIME_LIMIT_MS);
    for(int i=0, c=0;i<stun_count && c<5;i++){
        if(stun_list[i].rtt_ms >= 0 && stun_list[i].rtt_ms < TIME_LIMIT_MS){
            printf(" [%3ld ms] %s:%d\n", stun_list[i].rtt_ms,
                   stun_list[i].host, stun_list[i].port);
            c++;
        }
    }
    return 0;
}
