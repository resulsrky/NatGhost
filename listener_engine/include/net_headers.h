#ifndef NET_HEADERS_H
#define NET_HEADERS_H

#include <stdint.h>

// Bu başlık dosyası, ağ paketlerini analiz ederken kullanacağımız C yapılarını (struct) içerir.
// Windows'un kendi 'iphdr' tanımıyla çakışmayı önlemek için kendi özel struct isimlerimizi kullanıyoruz.

// Derleyicinin struct'lara ekstra byte eklemesini (padding) engelle
#pragma pack(push, 1)

// Kendi IP başlık yapımız
typedef struct natghost_iphdr {
    uint8_t ihl:4;
    uint8_t version:4;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
} natghost_iphdr_t;

// Kendi UDP başlık yapımız
typedef struct natghost_udphdr {
    uint16_t source;
    uint16_t dest;
    uint16_t len;
    uint16_t check;
} natghost_udphdr_t;

// ICMP başlığının temel yapısı (ilk 4 byte)
typedef struct natghost_icmphdr {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
} natghost_icmphdr_t;


#pragma pack(pop)

#endif //NET_HEADERS_H
