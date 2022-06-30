#ifndef _ARP_H_
#define _ARP_H_
#include <stdlib.h>
#include "structures/graph.h"
#include "structures/net.h"
#include "ethernet.h"
#include <sys/types.h>
#include "packet.h"

#define ETH_HRD_TYPE 0x1
#define REQ_OP 0x1
#define RESP_OP 0x2
#define RSLVD 1
#define UNRSLVD 0
#define RSLVD_ERR -1

#define ARP_MIN_SIZE ((sizeof(u_int16_t) * 3) + (sizeof(u_int8_t) * 2))

/*
Address Resolution Protocol is used to resolve IP adress to MAC address
when sending packets. If a packet is passed o L2 from L3 and the destionation IP
is not in the ARP table, the packet is buffered in an ARP buffer and an ARP request
is sent instead. The relevant receiving interface send a response message to the requester,
which update it's ARP table, create ethernet frame and finally send the packet.
*/
struct arp_packet
{
    u_int16_t hrd;
    u_int16_t pro;
    u_int8_t hln;
    u_int8_t pln;
    u_int16_t op;
    char *sha;
    char *spa;
    char *tha;
    char *tpa;   
};

int resolve_target_ip(struct interface *sender, u_int32_t target_ip, u_int8_t *target_mac);
void *create_arp_packet(struct interface *requester, u_int32_t target_ip, 
                            u_int8_t *target_mac, size_t *arp_size, int op);
int create_arp_structure(struct interface *requester, u_int32_t target_ip, u_int8_t *target_mac, int op, struct arp_packet *arp);
int get_arp_packet(struct arp_packet *arp, char *packet, size_t pckt_size);
int L2_pass_to_L3_arp(struct interface *rcv_itf, char * packet, size_t pckt_size);
void process_arp_request(struct interface *rcv_itf, struct arp_packet *arp);
void process_arp_response(struct interface *rcv_itf, struct arp_packet *arp);

int buffer_packet_arp(struct interface *sender, char *packet, size_t pckt_size, u_int32_t target_ip);
void add_arp_entry(struct interface *itf, u_int32_t ip, u_int8_t *mac);
void send_waiting_packet(struct interface *itf, u_int32_t ip);
void free_arp_request(struct arp_packet *arp);
size_t get_arp_size(struct arp_packet *arp);

void print_arp(struct arp_packet *arp);

#endif