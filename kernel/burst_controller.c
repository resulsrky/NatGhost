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
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>


#define MAX_STUN       100
#define BATCH_SIZE     64
#define MAGIC_COOKIE   0x2112A442
#define STUN_PKT_LEN   20
#define TIME_LIMIT_MS  100

typedef struct {
    char host[256];
    char public_ip[INET_ADDRSTRLEN];
    uint16_t public_port;
    uint16_t port;
    struct sockaddr_in addr;
    uint8_t transaction_id[12];
    struct timespec send_time;
    long rtt_ms;
    char ip_str[INET_ADDRSTRLEN];
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
    };
    int total = sizeof(servers)/sizeof(servers[0]);
    for(int i=0; i<total && stun_count<MAX_STUN; i++){
        struct addrinfo hints={0}, *res;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        if(getaddrinfo(servers[i], "3478", &hints, &res)==0) {
            strncpy(stun_list[stun_count].host, servers[i], sizeof(stun_list[0].host)-1);
            stun_list[stun_count].host[sizeof(stun_list[0].host)-1] = '\0';
            stun_list[stun_count].port = 3478;
            memcpy(&stun_list[stun_count].addr, res->ai_addr, sizeof(struct sockaddr_in));
            inet_ntop(AF_INET, &((struct sockaddr_in*)res->ai_addr)->sin_addr,
                      stun_list[stun_count].ip_str, sizeof(stun_list[0].ip_str));
            stun_list[stun_count].rtt_ms = -1;
            stun_list[stun_count].public_port = 0;
            stun_list[stun_count].public_ip[0] = '\0';
            freeaddrinfo(res);
            stun_count++;
        }
    }
}

int parse_xor_mapped_address(uint8_t *buf, int len, char *out_ip, uint16_t *out_port) {
if (len < 20) {
    printf(" Invalid STUN packet size: %d bytes\n", len);
    return -1;
}
    const uint8_t *attr = buf + 20;
    int remain = len - 20;
    while (remain >= 4) {
        uint16_t type = (attr[0] << 8) | attr[1];
        uint16_t alen = (attr[2] << 8) | attr[3];
        if (type == 0x0020 && alen >= 8) {
            uint8_t family = attr[5];
            printf("No XOR-MAPPED-ADDRESS attr found (type=%04x len=%d)\n", type, alen);

            if (family == 0x01) {
                uint16_t xor_port = (attr[6] << 8) | attr[7];
                *out_port = xor_port ^ ((MAGIC_COOKIE >> 16) & 0xFFFF);
                uint32_t xor_ip;
                memcpy(&xor_ip, attr + 8, 4);
                xor_ip ^= htonl(MAGIC_COOKIE);
                inet_ntop(AF_INET, &xor_ip, out_ip, INET_ADDRSTRLEN);
                return 0;
            }
        }
        int skip = 4 + alen;
        if (alen % 4) skip += 4 - (alen % 4);
        attr += skip;
        remain -= skip;
    }
    return -1;
}

void *sender_thread(void *arg) {
    int idx = *(int*)arg;
    free(arg);
    uint8_t pkt[STUN_PKT_LEN] = {0};
    pkt[0]=0; pkt[1]=1;
    uint32_t cookie = htonl(MAGIC_COOKIE);
    memcpy(pkt+4, &cookie, 4);
    for(int j=0;j<12;j++){
        pkt[8+j] = stun_list[idx].transaction_id[j] = rand() & 0xFF;
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
    struct epoll_event ev = {0}; ev.events = EPOLLIN; ev.data.fd = sock_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);
    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int timeout = TIME_LIMIT_MS;
    while (1) {
        int nfds = epoll_wait(epfd, &ev, 1, timeout);
        if (nfds > 0) {
            int got = recvmmsg(sock_fd, msgs, BATCH_SIZE, 0, NULL);
            clock_gettime(CLOCK_MONOTONIC, &now);
            for (int m=0; m<got; m++) {
                if (msgs[m].msg_len < STUN_PKT_LEN) continue;
                uint8_t *tid = bufs[m] + 8;
                for (int k=0; k<stun_count; k++) {
                    if (stun_list[k].rtt_ms != -1) continue;
                    if (memcmp(tid, stun_list[k].transaction_id, 12)==0) {
                        long ms = (now.tv_sec - stun_list[k].send_time.tv_sec)*1000 +
                                   (now.tv_nsec - stun_list[k].send_time.tv_nsec)/1000000;
                        pthread_mutex_lock(&lock);
                        stun_list[k].rtt_ms = ms;
                        // parse public IP:PORT from this buffer
                        parse_xor_mapped_address(bufs[m], msgs[m].msg_len,
                                                 stun_list[k].public_ip,
                                                 &stun_list[k].public_port);
     printf(" DEBUG ATTR: STUN response from %s (%s) => %s:%d\n",
       stun_list[k].host, stun_list[k].ip_str,
       stun_list[k].public_ip, stun_list[k].public_port);

                        pthread_mutex_unlock(&lock);
                        break;
                    }
                }
            }
        }
        struct timespec cur; clock_gettime(CLOCK_MONOTONIC, &cur);
        long elapsed = (cur.tv_sec - start.tv_sec)*1000 + (cur.tv_nsec - start.tv_nsec)/1000000;
        if (elapsed >= TIME_LIMIT_MS) break;
        timeout = TIME_LIMIT_MS - elapsed;
    }
    close(epfd);
}

int get_fastest_stun_index() {
    int best = -1;
    for (int i=0; i<stun_count; i++) {
        if (stun_list[i].rtt_ms >= 0) {
            if (best==-1 || stun_list[i].rtt_ms < stun_list[best].rtt_ms) best = i;
        }
    }
    return best;
}

void get_ordered_indices(int *ordered, int max) {
    int used[MAX_STUN] = {0};
    for (int i=0; i<max; i++) {
        int best=-1;
        for (int j=0; j<stun_count; j++) {
            if (!used[j] && stun_list[j].rtt_ms>=0) {
                if (best==-1 || stun_list[j].rtt_ms < stun_list[best].rtt_ms) best=j;
            }
        }
        ordered[i] = best;
        if (best!=-1) used[best]=1;
    }
}

// Dönen cevap geçerli mi?
static inline int is_valid_stun(int i) {
    // public_ip dolu ve public_port sıfırdan farklıysa geçerli
    return stun_list[i].public_ip[0] != '\0'
        && stun_list[i].public_port != 0;
}


int main() {
    srand(time(NULL));
    load_stun_servers();
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) { perror("socket"); return 1; }
    // set non-blocking via fcntl
    int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
    // bind ephemeral port
    struct sockaddr_in local = {0};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = 0;
    bind(sock_fd, (struct sockaddr*)&local, sizeof(local));
    // send in parallel
    pthread_t *senders = malloc(sizeof(pthread_t)*stun_count);
    for (int i=0; i<stun_count; i++) {
        int *arg = malloc(sizeof(int)); *arg=i;
        pthread_create(&senders[i], NULL, sender_thread, arg);
    }
    for (int i=0; i<stun_count; i++) pthread_join(senders[i], NULL);
    free(senders);
    // receive & parse
    listener_loop();
    close(sock_fd);
    // İlk burst tamamlandı; şimdi en hızlısı parse edilemediyse retry listesi hazırla
int retry_targets[MAX_STUN];
int retry_count = 0;

// Tüm sunucular arasında dön, parse başarısız ama RTT var ise ekle
for (int i = 0; i < stun_count; i++) {
    if (stun_list[i].rtt_ms >= 0 && !is_valid_stun(i)) {
        retry_targets[retry_count++] = i;
    }
}

    // sort by rtt
    for (int i=0; i<stun_count; i++) {
        for (int j=i+1; j<stun_count; j++) {
            if (stun_list[j].rtt_ms>=0 &&
                (stun_list[i].rtt_ms<0 || stun_list[j].rtt_ms<stun_list[i].rtt_ms)) {
                stun_ctx_t tmp = stun_list[i];
                stun_list[i] = stun_list[j];
                stun_list[j] = tmp;
            }
        }
    }
    // print
    printf("\nEn hızlı 5 STUN sunucusu (IP tabanlı):\n");
    for (int i=0, c=0; i<stun_count && c<5; i++) {
        if (stun_list[i].rtt_ms>=0) {
            printf(" [%3ld ms] %s (%s:%d) => %s:%d\n",
                   stun_list[i].rtt_ms,
                   stun_list[i].ip_str,
                   stun_list[i].host,
                   stun_list[i].port,
                   stun_list[i].public_ip,
                   stun_list[i].public_port);
            c++;
        }
    }
    // select fastest
    int best = get_fastest_stun_index();
    if (best>=0) {
        printf("\n✅ Seçilen STUN: %s (%s:%d) yanıt: %s:%d [%ld ms]\n",
               stun_list[best].ip_str,
               stun_list[best].host,
               stun_list[best].port,
               stun_list[best].public_ip,
               stun_list[best].public_port,
               stun_list[best].rtt_ms);
    } else {
        printf("\nHiç STUN cevabı alınamadı.\n");
    }
    // failover list if needed
    int ordered[5];
    get_ordered_indices(ordered, 5);
    printf("\nFailover listesi:\n");
    for (int i=0; i<5; i++) {
        int idx = ordered[i];
        if (idx>=0) {
            printf("  #%d - %s (%s:%d) => %s:%d [%ld ms]\n",
                   i+1,
                   stun_list[idx].ip_str,
                   stun_list[idx].host,
                   stun_list[idx].port,
                   stun_list[idx].public_ip,
                   stun_list[idx].public_port,
                   stun_list[idx].rtt_ms);
        }
printf("DEBUG: Writing to SHM: %s:%d\n", stun_list[best].public_ip, stun_list[best].public_port);

int shm_fd = shm_open("/natghost_shm", O_CREAT | O_RDWR, 0666);
ftruncate(shm_fd, 64);  // 64 byte yeterli
void* addr = mmap(0, 64, PROT_WRITE, MAP_SHARED, shm_fd, 0);

snprintf((char*)addr, 64, "%s:%d", stun_list[best].public_ip, stun_list[best].public_port);

munmap(addr, 64);
close(shm_fd);
    }
    return 0;
}
// Compile with: gcc -o stun_client stun_client.c -lpthread
// Run with: ./stun_client