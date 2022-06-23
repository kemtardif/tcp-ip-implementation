#include "ethernet.h"

int prepare_ethernet_packet(char *ethernet_packet,
                            char *data_pckt, 
                            size_t data_size, 
                            u_int8_t *src_mac, 
                            u_int8_t *dst_mac,
                            u_int16_t type,
                            size_t *snd_size)
{
    //Initialize ethernet_packet to zeros
    memset(ethernet_packet, 0, MAX_ETH_SIZE);
    *snd_size = 0;    
     //Sanitize
    if(!sanitize_data_pckt(data_pckt, data_size, snd_size))
        return 0;
    //Add ethernet headers 
    if(!append_eth_headers(ethernet_packet, data_pckt, src_mac, dst_mac, type, snd_size))
        return 0;
    return 1;
}
void *remove_ethernet_headers(struct eth_frame *eth_frame, char *pckt, size_t pckt_size)
{
    if(!eth_frame)
        return NULL;
    if(!get_eth_frame(eth_frame, pckt, pckt_size))
        return NULL;
    return eth_frame->data;
}
int  append_eth_headers(char *ethernet_packet,
                            char *data_pckt, 
                            u_int8_t *src_mac, 
                            u_int8_t *dst_mac,
                            u_int16_t type,
                            size_t *snd_size)
{
    int i;
    if(!ethernet_packet || !data_pckt || !src_mac || !dst_mac || !type || !snd_size)
        return 0; 
    //Copy destination MAC in packet
    for(i = 0; i < MAC_SIZE; i++)
    {
        memcpy(ethernet_packet, &dst_mac[i], sizeof(u_int8_t));
        ethernet_packet += sizeof(u_int8_t);
    }
    //Copy source MAC in packet
    for(i = 0; i < MAC_SIZE; i++)
    {
        memcpy(ethernet_packet, &src_mac[i], sizeof(u_int8_t));
        ethernet_packet += sizeof(u_int8_t);
    }
    //Copy type in packet
    memcpy(ethernet_packet, &type, sizeof(u_int16_t));
    ethernet_packet += sizeof(u_int16_t);
    //Copy data
    memcpy(ethernet_packet, data_pckt, *snd_size);
    *snd_size += ETH_HEADER_SIZE;  
    return 1;   
}

int get_eth_frame(struct eth_frame *eth_frame, char *eth_pckt, size_t eth_size)
{
    int i;
    if(!eth_frame || !eth_pckt || eth_size < ETH_HEADER_SIZE)
         return 0;

    //Copy destination MAC
    for(i = 0; i < MAC_SIZE; i++)
    {
        memcpy(&eth_frame->destination[i], eth_pckt, sizeof(u_int8_t));
        eth_pckt += sizeof(u_int8_t);
    }
    //Copy source MAC
    for(i = 0; i < MAC_SIZE; i++)
    {
        memcpy(&eth_frame->source[i], eth_pckt, sizeof(u_int8_t));
        eth_pckt += sizeof(u_int8_t);
    }
    //Copy type
    memcpy(&eth_frame->type, eth_pckt, sizeof(u_int16_t));
    eth_pckt += sizeof(u_int16_t);

    eth_frame->data = (void *)eth_pckt;
    eth_frame->data_size = eth_size - ETH_HEADER_SIZE;
    return 1;
}

/*
Ensure data has the right size i.e. between 46 and 1500 bytes. 
If initial data is smaller, add junk.
*/
int sanitize_data_pckt(char *data_pckt, size_t data_size, size_t *pck_size)
{
    size_t size;
    if(!data_pckt || !data_size || !pck_size)
        return 0;
    if((size = data_size) < MIN_DATA_SIZE)
        size = MIN_DATA_SIZE; //Add memset 0's as junk
    else if (size > MTU)
        size = MTU; //Remove surplus
    *pck_size = size;
    return 1;
}

void set_eth_frame(struct eth_frame *frame, struct interface *snd_itf, struct interface *attchd_itf, char *data_pckt, u_int16_t type, int is_broadcast)
{
    int i;

     for(i = 0; i < MAC_SIZE; i++)
    {
        frame->destination[i] = is_broadcast ? 0xFF : attchd_itf->mac_addr[i];
        frame->source[i] = snd_itf->mac_addr[i];
    }
    frame->type = type;
    frame->data = data_pckt;
}