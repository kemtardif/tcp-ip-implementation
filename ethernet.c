#include "ethernet.h"

int prepare_ethernet_packet(struct interface *itf,
                            char *snd_pckt,
                            size_t *snd_size,
                            u_int16_t type,
                            int is_broadcast)
{
     //Sanitize
    if(!sanitize_data_pckt(snd_pckt, snd_size))
        return 0;
    //Add ethernet headers
    if(!prepare_ethernet_headers(itf, snd_pckt, snd_size, type, is_broadcast))
        return 0;
    return 1;
}
void *process_ethernet_packet(struct graph_node *node, char *pckt, size_t *pckt_size, int *type)
{
    struct eth_frame eth_frame;
    if(!remove_ethernet_headers(&eth_frame, pckt, *pckt_size))
        return NULL;
    //Use eth_frame struct to inspect packet

    *type = eth_frame.type;
    *pckt_size = eth_frame.data_size;
    return eth_frame.data;
}
int prepare_ethernet_headers(struct interface *snd_itf, char *data_pckt, size_t *pckt_size, u_int16_t type, int is_broadcast)
{
    struct eth_frame frame;
    struct interface *attchd_itf;
    int i;
    if(!snd_itf || !data_pckt || !pckt_size || !type)
        return 0;
    if((attchd_itf = get_attached_interface(snd_itf)) == NULL)
        return 0;

    set_eth_frame(&frame, snd_itf, attchd_itf, data_pckt, type, is_broadcast);
    if(!append_eth_headers(&frame, data_pckt, pckt_size))
        return 0;
    return 1;       
}
int  append_eth_headers(struct eth_frame *eth_frame, char *data_pckt, size_t *pckt_size)
{
    int i;
    if(!eth_frame || !data_pckt || !pckt_size)
        return 0;

    //Shift data by size of ethernet header
    memmove(data_pckt + ETH_HEADER_SIZE, data_pckt, *pckt_size);   
    //Copy destination MAC in packet
    for(i = 0; i < MAC_SIZE; i++)
    {
        memcpy(data_pckt, &eth_frame->destination[i], sizeof(u_int8_t));
        data_pckt += sizeof(u_int8_t);
    }
    //Copy source MAC in packet
    for(i = 0; i < MAC_SIZE; i++)
    {
        memcpy(data_pckt, &eth_frame->source[i], sizeof(u_int8_t));
        data_pckt += sizeof(u_int8_t);
    }
    //Copy type in packet
    memcpy(data_pckt, &eth_frame->type, sizeof(u_int16_t));
    data_pckt += sizeof(u_int16_t);
    *pckt_size += ETH_HEADER_SIZE;  
    return 1;   
}

int remove_ethernet_headers(struct eth_frame *eth_frame, char *eth_pckt, size_t eth_size)
{
    int i;
    if(!eth_frame || !eth_pckt || !eth_size)
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
int sanitize_data_pckt(char *data_pckt, size_t *pck_size)
{
    size_t size;
    if(!data_pckt || !pck_size)
        return 0;
    size = *pck_size;
    if(size < MIN_DATA_SIZE)
        size = MIN_DATA_SIZE; //Add memset 0's as junk
    else if (size > MTU)
        size = MTU; //Remove surplus
    memmove(data_pckt, data_pckt, size);
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