#include "net.h"

void set_ip(struct ip_struct *ip_addr, u_int32_t ip, u_int8_t mask)
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

void set_mac(u_int8_t *mac_addr, u_int8_t *addr)
{
    int i;
    if (!mac_addr) 
        return;
    for(i = 0; i < MAC_SIZE; i++)
         mac_addr[i] = addr[i];    
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

void generate_mac_addr(u_int8_t *mac)
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
        mac[i] = rand0 % 255;
    }
}

void generate_broadcast_addr(u_int8_t *mac_addr)
{
    int i;

    if(!mac_addr)
        return;  
    for(i = 0; i < MAC_SIZE; i++)
        mac_addr[i] = 0xFF;
}

int is_broadcast(u_int8_t *mac_addr)
{
    int i;
    u_int8_t part;
    if(!mac_addr)
        return -1;
    
    for(i = 0; i < MAC_SIZE; i++)
    {
        if(mac_addr[i] != 0xFF)
            return 0;     
    }
    return 1;
}
int are_mac_equal(u_int8_t *mac1, u_int8_t *mac2)
{
    int i, f;
    if(!mac1 || !mac1)
        return 0;
    for(i = 0; i < MAC_SIZE; i++)
    {
        if(mac1[i] != mac2[i])
            return 0;
    }
    return 1;
}
