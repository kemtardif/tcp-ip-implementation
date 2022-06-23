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

char *ip_to_string(char * ip_str, size_t str_lnt, u_int32_t ip)
{
    u_int8_t parts[IP_SIZE];   
    if(!ip_str || !str_lnt)
        return NULL;

    parts[0] = (ip >> 24) & 0xFF;
    parts[1] = (ip >> 16) & 0xFF;
    parts[2] = (ip >> 8) & 0xFF;
    parts[3] = (ip >> 0) & 0xFF;

    if(snprintf(ip_str, str_lnt, "%i.%i.%i.%i",parts[0], parts[1], parts[2], parts[3]) == -1)
        return NULL;
    return ip_str;
}

char *mac_to_string(char* mac_str, size_t str_lnt, u_int8_t* mac)
{
    if(!mac_str || !str_lnt || !mac)
        return NULL;
    if(snprintf(mac_str, str_lnt, "%02X-%02X-%02X-%02X-%02X-%02X", 
                                mac[0],
                                mac[1],
                                mac[2],
                                mac[3],
                                mac[4],
                                mac[5]) == -1)
        return NULL;
    return mac_str;
}
