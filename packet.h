#ifndef _PACKET_H_
#define _PACKET_H_
#include <sys/types.h>
#include <stdlib.h>
#include "structures/graph.h"
#include "ethernet.h"

#define MTU 1500


//Prepare packet for transmission. Return pointer to packet.
void *prepare_packet(struct interface *snd_itf, 
                    char *data_pckt, 
                    size_t data_size, 
                    int is_broadcast, 
                    size_t *snd_size,
                    u_int16_t type);
int process_packet(struct graph_node *node, char *pckt, size_t pckt_size);
#endif