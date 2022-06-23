#include "packet.h"


void add_packet_to_send_queue(struct interface *snd_itf, char *packet, size_t pckt_size)
{
    char *send_pckt;
    struct send_packet *queue_item;
    if(!snd_itf || !packet || !pckt_size)
        return;
    if((queue_item = malloc(sizeof(struct send_packet))) == NULL)
         return;       
    if((send_pckt = malloc(pckt_size)) == NULL)
    {
        free(queue_item);
        return;
    }
    memcpy(send_pckt, packet, pckt_size);

    queue_item->packet = send_pckt;
    queue_item->pckt_size = pckt_size;
    queue_item->retransmit = 4;

    if(!push(snd_itf->send_queue, (void *)queue_item))
        printf("Sending queue full at interface %s. Packet dropped.\n", snd_itf->name);
}
void broadcast_to_send_queues(struct graph_node *node, struct interface *except, 
                                    char *packet, size_t pckt_size)
{
    int i;
    struct interface *send_to;
    if(!node || !packet || !pckt_size)
        return;
    for(i = 0; i < MAX_INTERFACE; i ++)
    {
        send_to = node->interfaces[i];
        if(!send_to || send_to == except)
            continue;
        add_packet_to_send_queue(send_to, packet, pckt_size);
    }
}



