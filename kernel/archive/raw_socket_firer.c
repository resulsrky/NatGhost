// raw_socket_firer.c

#include "raw_socket_firer.h"
#include "stun_packet.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "time_utils.h"

int fire_stun_packet(const char *target_ip, uint8_t trans_id[12]) {
    int sockfd;
    struct sockaddr_in servaddr;
    uint8_t packet[STUN_HEADER_SIZE];

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(3478);
    inet_pton(AF_INET, target_ip, &servaddr.sin_addr);

    construct_stun_binding_request(packet, trans_id);

    ssize_t sent = sendto(sockfd, packet, STUN_HEADER_SIZE, 0,
                          (const struct sockaddr *)&servaddr, sizeof(servaddr));
    if (sent != STUN_HEADER_SIZE) {
        perror("sendto failed");
        close(sockfd);
        return -1;
    }

    close(sockfd);
    return 0;
}
