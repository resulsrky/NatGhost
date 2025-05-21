#ifndef BLAST_CONTROLLER_H
#define BLAST_CONTROLLER_H

#include <stdint.h>

void record_send(uint8_t trans_id[12], double time_ms);
double get_send_time(uint8_t trans_id[12]);

#endif
