#ifndef NATGHOST_NETWORK_CORE_H
#define NATGHOST_NETWORK_CORE_H

// Gereksiz Windows kütüphanelerini dışarıda bırakarak derleme hatalarını önler.
#define WIN32_LEAN_AND_MEAN

#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

// Derleyicinin struct'lara ekstra byte eklemesini (padding) engelle
#pragma pack(push, 1)

// İsim çakışmasını önlemek için 'natghost_' öneki eklendi.
struct natghost_iphdr {
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
};

// İsim çakışmasını önlemek için 'natghost_' öneki eklendi.
struct natghost_udphdr {
    uint16_t source;
    uint16_t dest;
    uint16_t len;
    uint16_t check;
};

#pragma pack(pop)


// --- Fonksiyon Bildirimleri (Prototip) ---
int forge_udp_packet(char *packet_buffer, const char *src_ip, const char *dst_ip,
                     uint16_t src_port, uint16_t dst_port,
                     const char *data, uint16_t data_len);

// İşçi process'in çalıştıracağı fonksiyon
void run_worker(const char *source_ip, const char *dest_ip, uint16_t start_port, uint16_t end_port);

#endif //NATGHOST_NETWORK_CORE_H