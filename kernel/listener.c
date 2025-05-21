#include "listener.h"
#include "time_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>

#define STUN_HEADER_SIZE 20
#define RESPONSE_TIMEOUT_SEC 3

// get_send_time(): dışarıdan gelecek olan ID → gönderim zamanı eşleyicidir
int listen_for_stun_responses(StunReply *results, int max_results, double (*get_send_time)(uint8_t trans_id[12])) {
    int sockfd;
    struct sockaddr_in servaddr;
    uint8_t buffer[512];
    socklen_t len = sizeof(servaddr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        return -1;
    }

    // Non-blocking yap
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in bindaddr;
    memset(&bindaddr, 0, sizeof(bindaddr));
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(54321);  // gönderici ile aynı port
    bindaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (const struct sockaddr *)&bindaddr, sizeof(bindaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        return -1;
    }

    printf("[*] Listening for STUN responses for %d seconds...\n", RESPONSE_TIMEOUT_SEC);

    double start = get_timestamp_ms();
    int count = 0;

    while (get_timestamp_ms() - start < RESPONSE_TIMEOUT_SEC * 1000.0) {
        ssize_t recvlen = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&servaddr, &len);

        if (recvlen > 0) {
            printf("[!] Raw STUN reply (%ld bytes): ", recvlen);
            for (int i = 0; i < recvlen; i++) {
                printf("%02X ", buffer[i]);
            }
            printf("\n");
        }

        if (recvlen >= STUN_HEADER_SIZE) {
            uint16_t msg_type = (buffer[0] << 8) | buffer[1];
            if (msg_type == 0x0101) {  // Binding Response
                uint8_t *trans_id = buffer + 8;

                double sent_time = get_send_time(trans_id);
                double now = get_timestamp_ms();

                if (sent_time > 0) {
                    double rtt = now - sent_time;

                    char ipstr[64];
                    inet_ntop(AF_INET, &servaddr.sin_addr, ipstr, sizeof(ipstr));

                    printf("[\u2713] Reply from %s \u2192 RTT = %.2f ms\n", ipstr, rtt);

                    if (count < max_results) {
                        strcpy(results[count].ip, ipstr);
                        results[count].rtt_ms = rtt;
                        count++;
                    }
                }
            }
        }

        usleep(1000);  // CPU dostu kısa gecikme
    }

    close(sockfd);
    return count;
}
