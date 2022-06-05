#ifndef _NET_H_
#define _NET_H_

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>

#define MAC_SIZE 6
#define IP_SIZE 4

struct mac_addr {
    u_int8_t addr[MAC_SIZE];
};
struct ip_addr {
    unsigned int ip_set;
    u_int32_t ip_addr;
    u_int8_t mask;
};
//Networking properties for nodes
struct node_net 
{
    struct ip_addr ip_addr;
};
//Networking properties for interfaces
struct if_net 
{
    struct ip_addr ip_addr;
    struct mac_addr mac_addr;
};

/////////Network-related functions//////////////
void node_net_init(struct node_net *node_net);
void if_net_init(struct if_net *if_net);
void set_ip(struct ip_addr *ip_addr, u_int32_t ip, u_int8_t mask);
void set_mac(struct mac_addr *mac_addr, u_int8_t addr[MAC_SIZE]);
u_int32_t get_subnet(u_int32_t ip, u_int8_t mask_bits);
void generate_mac_addr(struct mac_addr *mac);
void generate_broadcast_addr(struct mac_addr *mac);
int is_broadcast(struct mac_addr *mac);

#endif