#include "switch.h"


void process_L2_switch(struct interface *rcv_itf, struct eth_frame *eth_frame, char *packet, size_t pckt_size)
{
    struct interface *send_to = NULL;
    char mac_str[MAX_MAC_STRING];
    int broadcast = 0;
    if(!rcv_itf || !eth_frame || !packet || !pckt_size)
        return;
    update_switch_table(rcv_itf, eth_frame);

    mac_to_string(mac_str, MAX_MAC_STRING, eth_frame->destination);
    if(is_broadcast(eth_frame->destination))
        broadcast = 1;
    else if((send_to = (struct interface *)get_value(rcv_itf->node->switch_table, mac_str)) == NULL)
        broadcast = 1;        
    else if (send_to == rcv_itf)
        return; //Drop packet if source mac = destination mac

    if(broadcast)
        broadcast_to_send_queues(rcv_itf->node, rcv_itf, packet, pckt_size);
    else
        add_packet_to_send_queue(send_to, packet, pckt_size);
}

void update_switch_table(struct interface *rcv_itf, struct eth_frame *eth_frame)
{
    struct graph_node *node;
    char mac_str[MAX_MAC_STRING];

    if(!rcv_itf|| !eth_frame)
        return;
    node = rcv_itf->node;
    //MAC to string key
    if((mac_to_string(mac_str, MAX_MAC_STRING, eth_frame->source)) == NULL)
        return;
    //Add item (MAC source, receiving interface) if not in switch table
    if(!get_value(node->switch_table, mac_str))
       set_value(node->switch_table, mac_str, rcv_itf, N_FREE);
}