#ifndef _IP_H_
#define _IP_H_
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include "structures/graph.h"
#include "structures/net.h"

#define IP_HEADER_WORDS 5
#define IP_WORD sizeof(u_int32_t)
#define MAX_IP_SIZE 1500
#define IPV4_VERSION 4
#define TCP_PROTOCOL 6
#define UDP_PROTOCOL 17

struct ip_packet
{
    u_int8_t version_ihl;
    u_int8_t dscp_ecn;
    u_int16_t total_length;
    u_int16_t identification;
    u_int16_t flags_offset;
    u_int8_t ttl;
    u_int8_t protocol;
    u_int16_t checksum;
    u_int32_t ip_src;
    u_int32_t ip_dst;
    void *data;
};

int prepare_ip_packet(char *ip_packet, char *data, size_t data_size, 
                        u_int32_t ip_src,
                        u_int32_t ip_dst,
                        size_t *ip_size);

/*
Retreive the IP headers from ip_packet and stored then in ip_packet struct
The data pointer will point to the inner data of ip_pckt
*/
int get_ip_packet(struct ip_packet *ip_packet, char *ip_pckt, size_t ip_size);

int sanitize_data_pckt_for_ip(char *data_pckt, size_t data_size, size_t *ip_size);
int append_ip_headers(char *ip_packet, char *data, 
                        u_int8_t version_ihl,
                        u_int8_t dscp_ecn,
                        u_int16_t total_length,
                        u_int16_t identification,
                        u_int16_t flags_offset,
                        u_int8_t ttl,
                        u_int8_t protocol,
                        u_int16_t checksum,
                        u_int32_t ip_src,
                        u_int32_t ip_dst,
                        size_t *ip_size);

int L2_pass_to_L3_ip(struct interface *rcv_itf, char * packet, size_t pckt_size);


#endif