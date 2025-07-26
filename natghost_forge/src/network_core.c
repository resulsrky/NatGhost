#include <natghost/network_core.h>
#include <natghost/checksum.h>
#include <natghost/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int forge_udp_packet(char *packet_buffer, const char *src_ip, const char *dst_ip,
                     uint16_t src_port, uint16_t dst_port,
                     const char *data, uint16_t data_len)
{
    if (src_ip == NULL || dst_ip == NULL || data == NULL || packet_buffer == NULL) {
        fprintf(stderr, "HATA: forge_udp_packet fonksiyonuna NULL arguman gonderildi.\n");
        return -1;
    }

    struct natghost_iphdr *iph = (struct natghost_iphdr *)packet_buffer;
    struct natghost_udphdr *udph = (struct natghost_udphdr *)(packet_buffer + sizeof(struct natghost_iphdr));

    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct natghost_iphdr) + sizeof(struct natghost_udphdr) + data_len);
    iph->id = htons(rand() % 65535);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_UDP;
    iph->saddr = inet_addr(src_ip);
    iph->daddr = inet_addr(dst_ip);
    iph->check = 0;

    udph->source = htons(src_port);
    udph->dest = htons(dst_port);
    udph->len = htons(sizeof(struct natghost_udphdr) + data_len);
    udph->check = 0;

    char *payload = packet_buffer + sizeof(struct natghost_iphdr) + sizeof(struct natghost_udphdr);
    memcpy(payload, data, data_len);

    iph->check = ip_checksum(iph, sizeof(struct natghost_iphdr));
    udph->check = udp_checksum(iph->saddr, iph->daddr, udph, sizeof(struct natghost_udphdr) + data_len);

    return sizeof(struct natghost_iphdr) + sizeof(struct natghost_udphdr) + data_len;
}

void run_worker(const char *source_ip, const char *dest_ip, uint16_t start_port, uint16_t end_port) {
    char packet_buffer[PACKET_BUFFER_SIZE];
    SOCKET raw_socket;
    struct sockaddr_in dest_addr;

    raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (raw_socket == INVALID_SOCKET) {
        return;
    }

    int optval = 1;
    if (setsockopt(raw_socket, IPPROTO_IP, IP_HDRINCL, (const char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
        closesocket(raw_socket);
        return;
    }

    for (uint16_t port = start_port; port <= end_port; port++) {
        if (port == 0) continue;

        int packet_size = forge_udp_packet(
            packet_buffer,
            source_ip,
            dest_ip,
            port,
            port,
            DEFAULT_PAYLOAD,
            strlen(DEFAULT_PAYLOAD)
        );

        if (packet_size > 0) {
            struct natghost_iphdr* iph = (struct natghost_iphdr*)packet_buffer;
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = 0;
            dest_addr.sin_addr.s_addr = iph->daddr;

            sendto(raw_socket, packet_buffer, packet_size, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        }
    }
    closesocket(raw_socket);
}