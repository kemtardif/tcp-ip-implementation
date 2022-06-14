#ifndef _ETHERNET_H_
#define _ETHERNET_H_

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "structures/net.h"
#include "structures/graph.h"

#define ETH_HEADER_SIZE (2 * MAC_SIZE * sizeof(u_int8_t) + sizeof(u_int16_t))
#define MIN_DATA_SIZE 46
#define MTU 1500
#define MAX_ETH_SIZE (ETH_HEADER_SIZE + MTU)

//Reprensent an ethernet frame
struct eth_frame
{
    u_int8_t destination[MAC_SIZE];
    u_int8_t source[MAC_SIZE];
    u_int16_t type;
    void *data; 
    size_t data_size;
};

int prepare_ethernet_packet(struct interface *itf,
                            char *snd_pckt,
                            size_t *snd_size,
                            u_int16_t type,
                            int is_broadcast);

//Extract and process ethernet headers, return pointer to data
void *process_ethernet_packet(struct graph_node *node, char *pckt, size_t *pckt_size, int *type);
/*
Construct L2-frame by appending ethernet headers, copying result in eth_pckt;
*/
int prepare_ethernet_headers(struct interface *snd_itf, 
                                char *data_pckt, 
                                size_t *pckt_size, 
                                u_int16_t type, 
                                int is_broadcast);
/*
Append eth_frame headers to given data_pckt, copying result in eth_pckt. 
Assume the data is already sanitized
*/
int append_eth_headers(struct eth_frame *eth_frame,
                             char *data_pckt, 
                             size_t *pckt_size);
/*
Remove ethernet headers from incoming packet and store them in struct. 
The data part is stored in data_pckt.
The data pointer in eth_frame points to data packet. 
*/
int remove_ethernet_headers(struct eth_frame *eth_frame, char *eth_pckt, size_t eth_size);
//Ensure data packet is between 46 and 1500 bytes
int sanitize_data_pckt(char *data_pckt, size_t *pck_size);
void set_eth_frame(struct eth_frame *frame, struct interface *snd_itf, struct interface *attchd_itf, char *data_pckt, u_int16_t type, int is_broadcast);

#endif