#include <natghost/network_core.h>
#include <natghost/checksum.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

uint16_t ip_checksum(void *vdata, size_t length) {
    char *data = vdata;
    uint32_t sum = 0;
    for (; length > 1; length -= 2) {
        sum += *(uint16_t *)data;
        data += 2;
    }
    if (length == 1) {
        sum += *(uint8_t *)data;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)~sum;
}

struct pseudo_header {
    uint32_t source_address;
    uint32_t dest_address;
    uint8_t placeholder;
    uint8_t protocol;
    uint16_t udp_length;
};

uint16_t udp_checksum(uint32_t source_ip, uint32_t dest_ip, void *udp_packet, size_t udp_packet_len) {
    struct pseudo_header psh;
    psh.source_address = source_ip;
    psh.dest_address = dest_ip;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = htons((uint16_t)udp_packet_len);

    size_t total_len = sizeof(struct pseudo_header) + udp_packet_len;
    char *checksum_buffer = malloc(total_len);
    if (!checksum_buffer) {
        fprintf(stderr, "HATA: Checksum buffer icin bellek ayrılamadı.\n");
        return 0;
    }

    memcpy(checksum_buffer, &psh, sizeof(struct pseudo_header));
    memcpy(checksum_buffer + sizeof(struct pseudo_header), udp_packet, udp_packet_len);

    uint16_t result = ip_checksum(checksum_buffer, total_len);

    free(checksum_buffer);
    return result;
}