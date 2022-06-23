#ifndef _PACKET_H_
#define _PACKET_H_
#include <sys/types.h>
#include <stdlib.h>
#include "structures/graph.h"
#include "structures/queue.h"

#define MTU 1500
//Will malloc a copy of the packet and add it to the send_buffer of the interface
void add_packet_to_send_queue(struct interface *snd_itf, char *packet, size_t pckt_size);
//Will malloc a packet for each interface on node not except and add to send_list
void broadcast_to_send_queues(struct graph_node *node, struct interface *except, char *packet, size_t pckt_size);

#endif