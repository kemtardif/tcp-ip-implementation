#include "net.h"

void node_net_init(struct node_net *node_net)
{
    node_net->ip_addr.ip_set = 0;
    node_net->ip_addr.ip_addr = 0;
    node_net->ip_addr.mask = 32;
}
void if_net_init(struct if_net *if_net)
{
    int i;
    if_net->ip_addr.ip_set = 0;
    if_net->ip_addr.ip_addr = 0x0;
    if_net->ip_addr.mask = 32;

    for(i = 0; i < MAC_SIZE; i++)
        if_net->mac_addr.addr[i] = 0;
}

void set_ip(struct ip_addr *ip_addr, u_int32_t ip, u_int8_t mask)
{
    if(!ip_addr)
        return;
    if(!ip)
        ip_addr->ip_set = 0;
    else
        ip_addr->ip_set = 1;
    ip_addr->ip_addr = ip;
    ip_addr->mask = mask;
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
