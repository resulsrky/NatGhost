#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#define MAGIC_COOKIE 0x2112A442
#define MAX_SERVERS 256

typedef struct {
    const char *host;
    uint16_t port;
    long rtt_ms;
} stun_server_t;

stun_server_t stun_servers[] = {
    {"stun.1und1.de", 3478}, {"stun.gmx.net", 3478},
    {"stun.stunprotocol.org", 3478}, {"stun.sipnet.net", 3478},
    {"stun.sipnet.ru", 3478}, {"stun.12connect.com", 3478},
    {"stun.12voip.com", 3478}, {"stun.3cx.com", 3478},
    {"stun.acrobits.cz", 3478}, {"stun.actionvoip.com", 3478},
    {"stun.advfn.com", 3478}, {"stun.altar.com.pl", 3478},
    {"stun.antisip.com", 3478}, {"stun.avigora.fr", 3478},
    {"stun.bluesip.net", 3478}, {"stun.cablenet-as.net", 3478},
    {"stun.callromania.ro", 3478}, {"stun.callwithus.com", 3478},
    {"stun.cheapvoip.com", 3478}, {"stun.cloopen.com", 3478},
    {"stun.counterpath.com", 3478}, {"stun.epygi.com", 3478},
    {"stun.ekiga.net", 3478}, {"stun.freeswitch.org", 3478},
    {"stun.ideasip.com", 3478}, {"stun.linphone.org", 3478},
    {"stun.voipbuster.com", 3478}, {"stun.voipcheap.com", 3478},
    {"stun.voipstunt.com", 3478}, {"stun.voipzoom.com", 3478},
    {"stun.zoiper.com", 3478}
    // Devamı kolayca eklenebilir, 256'ya kadar artırılabilir
};

int server_count = sizeof(stun_servers) / sizeof(stun_server_t);
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *stun_ping(void *arg) {
    int idx = *(int*)arg;
    free(arg);

    struct addrinfo hints = {0}, *res;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    char portstr[6];
    snprintf(portstr, sizeof(portstr), "%u", stun_servers[idx].port);
    if (getaddrinfo(stun_servers[idx].host, portstr, &hints, &res) != 0)
        return NULL;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        freeaddrinfo(res);
        return NULL;
    }

    struct sockaddr_in local = {0};
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    bind(sock, (struct sockaddr*)&local, sizeof(local));

    uint8_t req[20] = {0};
    req[0] = 0x00; req[1] = 0x01; // Binding Request
    req[4] = 0x21; req[5] = 0x12; req[6] = 0xA4; req[7] = 0x42;
    for (int i = 8; i < 20; i++) req[i] = rand() % 256;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    sendto(sock, req, sizeof(req), 0, res->ai_addr, res->ai_addrlen);

    struct timeval tv = {0, 200000}; // 200ms
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    uint8_t buf[1024];
    recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
    clock_gettime(CLOCK_MONOTONIC, &end);

    long rtt = (end.tv_sec - start.tv_sec) * 1000 +
               (end.tv_nsec - start.tv_nsec) / 1000000;

    pthread_mutex_lock(&lock);
    stun_servers[idx].rtt_ms = rtt;
    pthread_mutex_unlock(&lock);

    close(sock);
    freeaddrinfo(res);
    return NULL;
}

int main() {
    srand(time(NULL));
    pthread_t threads[MAX_SERVERS];

    for (int i = 0; i < server_count; i++) {
        int *arg = malloc(sizeof(int));
        *arg = i;
        pthread_create(&threads[i], NULL, stun_ping, arg);
    }

    for (int i = 0; i < server_count; i++)
        pthread_join(threads[i], NULL);

    // Sırala
    for (int i = 0; i < server_count; i++) {
        for (int j = i + 1; j < server_count; j++) {
            if (stun_servers[j].rtt_ms > 0 &&
               (stun_servers[i].rtt_ms == 0 || stun_servers[j].rtt_ms < stun_servers[i].rtt_ms)) {
                stun_server_t tmp = stun_servers[i];
                stun_servers[i] = stun_servers[j];
                stun_servers[j] = tmp;
            }
        }
    }

    printf("\nEn hızlı 5 STUN sunucusu:\n");
    for (int i = 0, count = 0; i < server_count && count < 5; i++) {
        if (stun_servers[i].rtt_ms > 0) {
            printf(" [%ld ms] %s:%d\n", stun_servers[i].rtt_ms,
                   stun_servers[i].host, stun_servers[i].port);
            count++;
        }
    }

    return 0;
}
