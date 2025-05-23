// sniff_listener.c - Promiscuous UDP packet listener for NATGhost burst

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUF_SIZE 2048
#define PAYLOAD_PREFIX "PORT"
#define PREFIX_LEN 4
#define PORT_STR_LEN 5

void set_promiscuous(int sock, const char *iface) {
    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface, IFNAMSIZ);
    ioctl(sock, SIOCGIFFLAGS, &ifr);
    ifr.ifr_flags |= IFF_PROMISC;
    ioctl(sock, SIOCSIFFLAGS, &ifr);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Kullanım: %s <interface>\n", argv[0]);
        exit(1);
    }

    const char *iface = argv[1];
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    set_promiscuous(sock, iface);

    uint8_t buf[BUF_SIZE];
    while (1) {
        ssize_t len = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
        if (len <= 0) continue;

        struct ether_header *eth = (struct ether_header *)buf;
        if (ntohs(eth->ether_type) != ETHERTYPE_IP) continue;

        struct iphdr *ip = (struct iphdr *)(buf + sizeof(struct ether_header));
        if (ip->protocol != IPPROTO_UDP) continue;

        size_t iphdr_len = ip->ihl * 4;
        struct udphdr *udp = (struct udphdr *)((uint8_t *)ip + iphdr_len);
        uint8_t *payload = (uint8_t *)(udp + 1);
        size_t payload_len = ntohs(udp->len) - sizeof(struct udphdr);

        if (payload_len >= PREFIX_LEN + PORT_STR_LEN && strncmp((char *)payload, PAYLOAD_PREFIX, PREFIX_LEN) == 0) {
            char port_str[PORT_STR_LEN + 1] = {0};
            memcpy(port_str, payload + PREFIX_LEN, PORT_STR_LEN);
            int reported_port = atoi(port_str);

            struct in_addr src_ip;
            src_ip.s_addr = ip->saddr;

            printf("[!] Çakışma: %s:%d --> %d (payload: PORT%d)\n",
                   inet_ntoa(src_ip), ntohs(udp->source), ntohs(udp->dest), reported_port);
        }
    }

    close(sock);
    return 0;
}
