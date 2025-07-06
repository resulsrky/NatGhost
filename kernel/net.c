#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "net.h"

#define UDP_PROTO 17 


uint16_t checksum(const void *data, uint32_t len)
{
    const uint16_t *buf = (const uint16_t *)data;
    uint32_t sum = 0;
    while (len > 1) {
        sum += *buf++;
        len -= 2;
    }
    if (len) {
        sum += *(const uint8_t *)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (uint16_t)(~sum);
}

size_t forge_udp_packet(uint8_t *packet_buf,
                        uint32_t src_ip,
                        uint16_t src_port,
                        uint32_t dst_ip,
                        uint16_t dst_port,
                        const uint8_t *payload,
                        uint32_t payload_len)
{
    ip_header  *ip  = (ip_header *) packet_buf;
    udp_header *udp = (udp_header *)(packet_buf + sizeof(ip_header));
    uint8_t    *data= packet_buf + sizeof(ip_header) + sizeof(udp_header);

    udp->src_port = htons(src_port);
    udp->dst_port = htons(dst_port);
    udp->len      = htons((uint16_t)(sizeof(udp_header) + payload_len));
    udp->cksum    = 0;

    if (payload_len)
        memcpy(data, payload, payload_len);

    ip->ver_ihl   = 0x45;
    ip->tos       = 0;
    ip->total_len = htons((uint16_t)(sizeof(ip_header) + sizeof(udp_header) + payload_len));
    ip->id        = htons(0);
    ip->frag_off  = htons(0);
    ip->ttl       = 64;
    ip->proto     = UDP_PROTO;
    ip->hdr_cksum = 0;
    ip->src_ip    = htonl(src_ip);
    ip->dst_ip    = htonl(dst_ip);

    ip->hdr_cksum = checksum(ip, sizeof(ip_header));
    pseudo_header ph;
    ph.src_addr = ip->src_ip;
    ph.dst_addr = ip->dst_ip;
    ph.zero     = 0;
    ph.proto    = UDP_PROTO;
    ph.udp_len  = udp->len;

    uint32_t pseudo_len = sizeof(pseudo_header) + sizeof(udp_header) + payload_len;
    uint8_t temp[pseudo_len];
    memcpy(temp, &ph, sizeof(pseudo_header));
    memcpy(temp + sizeof(pseudo_header), udp, sizeof(udp_header) + payload_len);

    uint16_t u_ck = checksum(temp, pseudo_len);
    udp->cksum = u_ck ? u_ck : 0xFFFF;

    return sizeof(ip_header) + sizeof(udp_header) + payload_len;
}
size_t forge_eth_udp(uint8_t *p,
                     const uint8_t dst_mac[6],
                     const uint8_t src_mac[6],
                     uint32_t sip, uint16_t sport,
                     uint32_t dip, uint16_t dport,
                     const uint8_t *pl, uint32_t plen)
{
    memcpy(p, dst_mac, 6);
    memcpy(p + 6, src_mac, 6);
    p[12] = 0x08;
    p[13] = 0x00;

    size_t l = forge_udp_packet(p + 14, sip, sport, dip, dport, pl, plen);
    size_t frame_len = 14 + l;
    if (frame_len < 60) {
        memset(p + frame_len, 0, 60 - frame_len);
        frame_len = 60;
    }
    return frame_len;
}


