#include "stun_packet.h"
#include "raw_socket_firer_threads.h"
#include "listener.h"
#include "time_utils.h"
#include "blast_controller.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

// Basit gönderim zamanı haritası
#define MAX_TARGETS 1024
typedef struct {
    uint8_t trans_id[12];
    double send_time;
} SendEntry;

SendEntry send_table[MAX_TARGETS];
int send_table_size = 0;

void record_send(uint8_t trans_id[12], double time_ms) {
    if (send_table_size < MAX_TARGETS) {
        memcpy(send_table[send_table_size].trans_id, trans_id, 12);
        send_table[send_table_size].send_time = time_ms;
        send_table_size++;
    }
}

double get_send_time(uint8_t trans_id[12]) {
    for (int i = 0; i < send_table_size; i++) {
        if (memcmp(send_table[i].trans_id, trans_id, 12) == 0) {
            return send_table[i].send_time;
        }
    }
    return -1;
}

typedef struct {
    const char *ip;
    uint8_t trans_id[12];
} Target;

Target targets[MAX_TARGETS];
int target_count = 0;

int load_static_ips(const char **out_array) {
    const char *static_ips[] = {
        "74.125.140.127",    // stun.l.google.com
        "62.146.106.170",    // stun.1und1.de
        "212.227.15.195",    // stun.gmx.net
        "212.227.124.134",   // stun.schlund.de
        "178.237.36.52",     // stun.rolmail.net
        "195.185.214.194"    // stun.rockenstein.de
    };
    int count = sizeof(static_ips) / sizeof(static_ips[0]);
    for (int i = 0; i < count; i++) {
        out_array[i] = static_ips[i];
    }
    return count;
}
int main() {
    srand(time(NULL));
    printf("[*] NATGhost STUN Discovery Engine\n");

    const char *ip_array[MAX_TARGETS];
    int target_count = load_static_ips(ip_array);

    printf("[*] Firing stun packets to %d targets...\n", target_count);
    fire_stun_to_all(ip_array, target_count);  // her IP için çoklu port patlaması yapılacak

    StunReply results[MAX_TARGETS];
    int found = listen_for_stun_responses(results, MAX_TARGETS, get_send_time);

    // Sırala
    for (int i = 0; i < found - 1; i++) {
        for (int j = i + 1; j < found; j++) {
            if (results[j].rtt_ms < results[i].rtt_ms) {
                StunReply tmp = results[i];
                results[i] = results[j];
                results[j] = tmp;
            }
        }
    }

    // Göster
    printf("\n[✓] Top STUN Servers by RTT:\n");
    for (int i = 0; i < found && i < 5; i++) {
        printf("• %s → %.2f ms\n", results[i].ip, results[i].rtt_ms);
    }

    return 0;
}

