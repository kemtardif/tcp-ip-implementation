#ifndef _SWITCH_H_
#define _SWITCH_H_

#include "structures/net.h"
#include "structures/graph.h"
#include "packet.h"
#include "ethernet.h"
/*
    A switch is a simple device used to connect multiple links
    in a LAN. It doesn't buffer packet or use any protocol, it just
    forward packet using its switch table.
*/
void process_switch(struct interface *rcv_itf, struct eth_frame *eth_frame, char *packet, size_t pckt_size);
/*

    Self-learning switching table. When a packet arrive
    on one of its interface, it add a (source MAC, receiving interface)
    entry in its table. To forward packet, it search it's switching table
    for an entry (destination MAC, interface) and pass the packet to the
    interface. Otherwise, it broadcasts except on the receiving interface.
*/
void update_switch_table(struct interface *rcv_itf, struct eth_frame *eth_frame);
#endif