#include "packet.h"
//Construct packet by adding layers of headers.
void *prepare_packet(struct interface *snd_itf, 
                    char *data_pckt, 
                    size_t data_size, 
                    int is_broadcast, 
                    size_t *snd_size,
                    u_int16_t type)
{
    char *snd_pckt;
    if(!snd_itf || !data_pckt || !data_size || !snd_size || !type)
        return NULL;
    //Ethernet protocol determine max size of packets (1 MTU)
    if((snd_pckt = malloc(MAX_ETH_SIZE)) == NULL)
        return NULL;

    //Copy initial data into sending packet;
    memset((void *)snd_pckt, 0, MAX_ETH_SIZE);
    if(data_size > MAX_ETH_SIZE)
        data_size = MAX_ETH_SIZE;
    memcpy(snd_pckt, data_pckt, data_size);
    *snd_size = data_size;

    //each layer do their thing on buffer free snd_pckt in case of error
    if(!prepare_ethernet_packet(snd_itf, snd_pckt, snd_size, type, is_broadcast))
    {
        free(snd_pckt);
        return NULL;
    }
    return snd_pckt;
}

int process_packet(struct graph_node *node, char *pckt, size_t pckt_size)
{
    char *data;
    size_t data_size;
    int type;

    if(!node || !pckt|| !pckt_size)
         return 0;  
    data = pckt;
    data_size = pckt_size;

    if((data = process_ethernet_packet(node, data, &data_size, &type)) == NULL)
        return 0;
    return 1;
}

