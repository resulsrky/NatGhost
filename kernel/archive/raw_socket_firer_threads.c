#include "raw_socket_firer_threads.h"
#include "stun_packet.h"
#include "time_utils.h"
#include "blast_controller.h"  // record_send tanımı için

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define STUN_HEADER_SIZE 20
#define MAX_THREADS 1024
#define STUN_SRC_PORT 54321  // listener ile aynı port

void record_send(uint8_t trans_id[12], double time_ms);

typedef struct {
    const char *ip;
    uint8_t trans_id[12];
    uint16_t port;
    int sockfd;
} ThreadArgs;

void *fire_thread(void *arg) {
    ThreadArgs *targs = (ThreadArgs *)arg;
    const char *ip = targs->ip;
    int sockfd = targs->sockfd;

    struct sockaddr_in servaddr;
    uint8_t packet[STUN_HEADER_SIZE];

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(targs->port);
    inet_pton(AF_INET, ip, &servaddr.sin_addr);

    // STUN paketini oluştur
    construct_stun_binding_request(packet, targs->trans_id);

    double send_time = get_timestamp_ms();
    ssize_t sent = sendto(sockfd, packet, STUN_HEADER_SIZE, 0,
                          (const struct sockaddr *)&servaddr, sizeof(servaddr));
    if (sent == STUN_HEADER_SIZE) {
        printf("[\u2022] Sent to %s:%d at %.2f ms\n", ip, targs->port, send_time);
    }

    pthread_exit(NULL);
}

void fire_stun_to_all(const char **ip_list, int ip_count) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return;
    }

    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        perror("setsockopt failed");
        close(sockfd);
        return;
    }

    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(STUN_SRC_PORT);

    if (bind(sockfd, (struct sockaddr *)&local, sizeof(local)) < 0) {
        perror("bind() to source port failed");
        close(sockfd);
        return;
    }

    pthread_t threads[MAX_THREADS];
    ThreadArgs args[MAX_THREADS];

    const uint16_t stun_ports[] = {3478, 19302, 19305, 3479, 5349, 443};
    const int port_count = sizeof(stun_ports) / sizeof(stun_ports[0]);

    int thread_index = 0;
    srand(time(NULL));

    for (int i = 0; i < ip_count; i++) {
        for (int j = 0; j < port_count; j++) {
            if (thread_index >= MAX_THREADS) break;

            args[thread_index].ip = ip_list[i];
            args[thread_index].port = stun_ports[j];
            args[thread_index].sockfd = sockfd;
            generate_transaction_id(args[thread_index].trans_id);
            record_send(args[thread_index].trans_id, get_timestamp_ms());

            pthread_create(&threads[thread_index], NULL, fire_thread, &args[thread_index]);
            thread_index++;
        }
    }

    for (int i = 0; i < thread_index; i++) {
        pthread_join(threads[i], NULL);
    }

    close(sockfd);
}
