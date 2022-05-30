#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include "net.h"

void node_net_init(struct node_net *node_net)
{
    node_net->ip_addr.ip_set = 0;
    node_net->ip_addr.ip_addr = 0;
}
void if_net_init(struct if_net *if_net)
{
    int i;
    if_net->ip_addr.ip_set = 0;
    if_net->ip_addr.ip_addr = 0x0;

    for(i = 0; i < MAC_SIZE; i++)
        if_net->mac_addr.addr[i] = 0;
    if_net->mask = 0;
}

void set_ip(struct ip_addr *ip_addr, u_int32_t ip)
{
    if(!ip_addr)
        return;
    if(!ip)
        ip_addr->ip_set = 0;
    else
        ip_addr->ip_set = 1;
    ip_addr->ip_addr = ip;
}

void set_mac(struct mac_addr *mac_addr, u_int8_t addr[MAC_SIZE])
{
    int i;
    if (!mac_addr) 
        return;
    for(i = 0; i < MAC_SIZE; i++)
        mac_addr->addr[i] = addr[i];
}

u_int32_t get_subnet(u_int32_t ip, u_int8_t mask_bits)
{
    u_int32_t mask;
    size_t rs;

    rs = sizeof(u_int32_t) << 3;
    rs -= mask_bits;
    mask = ~((1 << rs) - 1);
    mask = ip & mask;

    return mask;    
}

void generate_mac_addr(struct mac_addr *mac)
{
    int i;
    long rand0, rand1, rand2, rand3;

    if(!mac)
        return;
    for(i = 0; i < MAC_SIZE; i++)
    {
        rand1 = rand();
        rand2 = rand();
        rand3 = rand();
        rand0 = rand1 * rand2 * rand3;
        mac->addr[i] = rand0 % 255;
    }
}

void generate_broadcast_addr(struct mac_addr *mac)
{
    int i;

    if(!mac)
        return;
    
    for(i = 0; i < MAC_SIZE; i++)
        mac->addr[i] = 0xFF;
}

int is_broadcast(struct mac_addr *mac)
{
    int i;
    u_int8_t part;
    if(!mac)
        return -1;
    
    for(i = 0; i < MAC_SIZE; i++)
    {
        part = mac->addr[i];
        if(part != 0xFF)
            return 0;     
    }
    return 1;
}


void print_ip(struct ip_addr *ip_addr)
{
    int i;
    u_int8_t parts[IP_SIZE];
    u_int32_t ip = ip_addr->ip_addr;

    if(!ip_addr)
        return;

    parts[0] = (ip >> 24) & 0xFF;
    parts[1] = (ip >> 16) & 0xFF;
    parts[2] = (ip >> 8) & 0xFF;
    parts[3] = (ip >> 0) & 0xFF;

    printf("IP address : %i.%i.%i.%i\n", parts[0], parts[1], parts[2], parts[3]);
}

void print_mac(struct mac_addr *mac_addr)
{
    if(!mac_addr)
        return;
    printf("MAC address : %X-%X-%X-%X-%X-%X\n", mac_addr->addr[0],
                                mac_addr->addr[1],
                                mac_addr->addr[2],
                                mac_addr->addr[3],
                                mac_addr->addr[4],
                                mac_addr->addr[5]);
}
