ifndef NET_H
#define NET_H


#include <stdint.h>
#include <stddef.h>
#include "bump_alloc.h"
#define NUM_TX_DESC  1024
#define TX_BUF_SIZE   2048

typedef struct __attribute__((packed))
{
    uint32_t src_addr;
    uint32_t dst_addr;
    uint8_t  zero; 
    uint8_t  proto;  
    uint16_t udp_len;
} pseudo_header;

typedef struct __attribute__((packed))
{
    uint8_t  ver_ihl;      /* 0x45 => IPv4, 5*4 = 20 B header */
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t hdr_cksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} ip_header;

typedef struct __attribute__((packed))
{
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t cksum;
} udp_header;

uint16_t checksum(const void *data, uint32_t len);

#endif
