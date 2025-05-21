#ifndef LISTENER_H
#define LISTENER_H

#include <stdint.h>

typedef struct {
    char ip[64];
    double rtt_ms;
} StunReply;

int listen_for_stun_responses(StunReply *results, int max_results, double (*get_send_time)(uint8_t trans_id[12]));

#endif
