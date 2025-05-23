#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <pthread.h>

#define MAX_STUN       100
#define BATCH_SIZE     64
#define MAGIC_COOKIE   0x2112A442
#define STUN_PKT_LEN   20
#define TIME_LIMIT_MS  100
#define SHM_NAME       "/natghost_shm"
#define SHM_SIZE       64

typedef struct {
    char host[256];
    char ip_str[INET_ADDRSTRLEN];
    uint16_t port;
    struct sockaddr_in addr;
    uint8_t transaction_id[12];
    struct timespec send_time;
    long rtt_ms;
    char public_ip[INET_ADDRSTRLEN];
    uint16_t public_port;
} stun_ctx_t;

static stun_ctx_t stun_list[MAX_STUN];
static int stun_count = 0;
static int sock_fd;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static int cmp_rtt(const void *pa, const void *pb) {
    const stun_ctx_t *a = pa;
    const stun_ctx_t *b = pb;
    if (a->rtt_ms < 0 && b->rtt_ms < 0) return 0;
    if (a->rtt_ms < 0) return 1;
    if (b->rtt_ms < 0) return -1;
    return (a->rtt_ms > b->rtt_ms) - (a->rtt_ms < b->rtt_ms);
}

void load_stun_servers() {
    const char *servers[] = {
        "stun.voipbuster.com", "stun.voipstunt.com", "stun.voipzoom.com",
        "stun.linphone.org",  "stun.antisip.com",   "stun.gmx.net",
        "stun.sipnet.net",    "stun.12voip.com",    "stun.3cx.com",
        "stun.ekiga.net"
    };
    int total = sizeof(servers)/sizeof(servers[0]);
    for(int i=0; i<total && stun_count<MAX_STUN; i++){
        struct addrinfo hints={0}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        if(getaddrinfo(servers[i], "3478", &hints, &res)==0) {
            stun_ctx_t *ctx = &stun_list[stun_count];
            strncpy(ctx->host, servers[i], sizeof(ctx->host)-1);
            ctx->host[sizeof(ctx->host)-1] = '\0';
            ctx->port = 3478;
            memcpy(&ctx->addr, res->ai_addr, sizeof(ctx->addr));
            inet_ntop(AF_INET, &ctx->addr.sin_addr, ctx->ip_str, sizeof(ctx->ip_str));
            ctx->rtt_ms = -1;
            ctx->public_ip[0] = '\0';
            ctx->public_port = 0;
            freeaddrinfo(res);
            stun_count++;
        }
    }
}

int parse_xor_mapped_address(uint8_t *buf, int len, char *out_ip, uint16_t *out_port) {
    if (len < 20) return -1;
    uint8_t *attr = buf + 20;
    int remain = len - 20;
    while(remain >= 4) {
        uint16_t type = (attr[0]<<8) | attr[1];
        uint16_t alen = (attr[2]<<8) | attr[3];
        if(type == 0x0020 && alen >= 8) {
            uint8_t family = attr[5];
            if(family == 0x01) {
                uint16_t xor_port = (attr[6]<<8) | attr[7];
                *out_port = xor_port ^ (MAGIC_COOKIE >> 16);
                uint32_t xor_ip;
                memcpy(&xor_ip, attr+8, 4);
                xor_ip ^= htonl(MAGIC_COOKIE);
                inet_ntop(AF_INET, &xor_ip, out_ip, INET_ADDRSTRLEN);
                return 0;
            }
        }
        int skip = 4 + alen;
        if(alen % 4) skip += 4 - (alen % 4);
        attr += skip;
        remain -= skip;
    }
    return -1;
}

static inline int is_valid_stun(int i) {
    return stun_list[i].public_ip[0] != '\0' && stun_list[i].public_port != 0;
}

void send_request(int idx) {
    uint8_t pkt[STUN_PKT_LEN] = {0};
    pkt[0]=0; pkt[1]=1;
    uint32_t cookie = htonl(MAGIC_COOKIE);
    memcpy(pkt+4, &cookie, 4);
    for(int j=0;j<12;j++) {
        pkt[8+j] = stun_list[idx].transaction_id[j] = rand() & 0xFF;
    }
    clock_gettime(CLOCK_MONOTONIC, &stun_list[idx].send_time);
    sendto(sock_fd, pkt, STUN_PKT_LEN, 0, (struct sockaddr*)&stun_list[idx].addr, sizeof(stun_list[idx].addr));
}

void burst_send_all(int *targets, int count) {
    for(int i=0;i<count;i++) send_request(targets[i]);
}

void burst_receive(int *targets, int count) {
    struct mmsghdr msgs[BATCH_SIZE];
    struct iovec iovecs[BATCH_SIZE];
    uint8_t bufs[BATCH_SIZE][512];
    struct sockaddr_in froms[BATCH_SIZE];
    struct timespec now;
    for(int i=0;i<BATCH_SIZE;i++){
        iovecs[i].iov_base = bufs[i];
        iovecs[i].iov_len = sizeof(bufs[i]);
        msgs[i].msg_hdr.msg_iov = &iovecs[i];
        msgs[i].msg_hdr.msg_iovlen = 1;
        msgs[i].msg_hdr.msg_name = &froms[i];
        msgs[i].msg_hdr.msg_namelen = sizeof(froms[i]);
        msgs[i].msg_hdr.msg_control = NULL;
        msgs[i].msg_hdr.msg_controllen = 0;
        msgs[i].msg_len = 0;
    }
    int epfd = epoll_create1(0);
    struct epoll_event ev = {.events=EPOLLIN, .data.fd=sock_fd};
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int timeout = TIME_LIMIT_MS;
    while(1) {
        int nfds = epoll_wait(epfd, &ev, 1, timeout);
        if(nfds>0) {
            int got = recvmmsg(sock_fd, msgs, BATCH_SIZE, 0, NULL);
            clock_gettime(CLOCK_MONOTONIC, &now);
            for(int m=0;m<got;m++){
                if(msgs[m].msg_len < STUN_PKT_LEN) continue;
                uint8_t *tid = bufs[m] + 8;
                for(int ti=0; ti<count; ti++){
                    int k = targets[ti];
                    if(stun_list[k].rtt_ms != -1) continue;
                    if(memcmp(tid, stun_list[k].transaction_id,12)==0) {
                        long ms = (now.tv_sec - stun_list[k].send_time.tv_sec)*1000 +
                                   (now.tv_nsec - stun_list[k].send_time.tv_nsec)/1000000;
                        pthread_mutex_lock(&lock);
                        stun_list[k].rtt_ms = ms;
                        parse_xor_mapped_address(bufs[m], msgs[m].msg_len,
                                                 stun_list[k].public_ip,
                                                 &stun_list[k].public_port);
                        pthread_mutex_unlock(&lock);
                        break;
                    }
                }
            }
        }
        struct timespec cur;
        clock_gettime(CLOCK_MONOTONIC,&cur);
        long elapsed = (cur.tv_sec - start.tv_sec)*1000 + (cur.tv_nsec - start.tv_nsec)/1000000;
        if(elapsed >= TIME_LIMIT_MS) break;
        timeout = TIME_LIMIT_MS - elapsed;
    }
    close(epfd);
}

void write_shared_memory(int best) {
    int fd = shm_open(SHM_NAME, O_CREAT|O_RDWR, 0666);
    ftruncate(fd, SHM_SIZE);
    void *mem = mmap(NULL, SHM_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
    snprintf(mem, SHM_SIZE, "%s:%d",
             stun_list[best].public_ip,
             stun_list[best].public_port);
    munmap(mem, SHM_SIZE);
    close(fd);
}

int main() {
    srand(time(NULL));
    load_stun_servers();
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
    struct sockaddr_in local = {.sin_family=AF_INET, .sin_addr.s_addr=INADDR_ANY, .sin_port=0};
    bind(sock_fd, (struct sockaddr*)&local, sizeof(local));

    int initial_targets[MAX_STUN];
    for(int i=0;i<stun_count;i++) initial_targets[i] = i;
    burst_send_all(initial_targets, stun_count);
    burst_receive(initial_targets, stun_count);

    int retry_targets[MAX_STUN], retry_count=0;
    for(int i=0;i<stun_count;i++){
        if(stun_list[i].rtt_ms>=0 && !is_valid_stun(i))
            retry_targets[retry_count++] = i;
    }
    if(retry_count>0) {
        burst_send_all(retry_targets, retry_count);
        burst_receive(retry_targets, retry_count);
    }

    qsort(stun_list, stun_count, sizeof(stun_ctx_t), cmp_rtt);

    printf("En hızlı 5 STUN sunucusu (IP tabanlı):\n");
    int printed=0;
    for(int i=0;i<stun_count && printed<5;i++){
        if(stun_list[i].rtt_ms>=0) {
            printf(" [%3ld ms] %s (%s:%d) => %s:%d\n",
                   stun_list[i].rtt_ms,
                   stun_list[i].host,
                   stun_list[i].ip_str,
                   stun_list[i].port,
                   stun_list[i].public_ip,
                   stun_list[i].public_port);
            printed++;
        }
    }

    int best = -1;
    for(int i=0;i<stun_count;i++){
        if(is_valid_stun(i)) { best = i; break; }
    }
    if(best>=0) {
        printf("\nSe\xC3\xA7ilen STUN: %s (%s:%d) yan\xC4\xB1t: %s:%d [%ld ms]\n",
               stun_list[best].host,
               stun_list[best].ip_str,
               stun_list[best].port,
               stun_list[best].public_ip,
               stun_list[best].public_port,
               stun_list[best].rtt_ms);
        write_shared_memory(best);
    } else {
        printf("\n Hi\xC3\xA7 ge\xC3\xA7erli STUN cevab\xC4\xB1 al\xC4\xB1namad\xC4\xB1.\n");
    }

    return 0;
}
