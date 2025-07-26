#ifndef NATGHOST_CHECKSUM_H
#define NATGHOST_CHECKSUM_H

#include <stdint.h>
#include <stddef.h>

uint16_t ip_checksum(void *vdata, size_t length);

uint16_t udp_checksum(uint32_t source_ip, uint32_t dest_ip, void *udp_packet, size_t udp_packet_len);

#endif //NATGHOST_CHECKSUM_H