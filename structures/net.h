#ifndef _NET_H_
#define _NET_H_

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>

#define MAC_SIZE 6
#define IP_SIZE 4
#define MAX_IP_STRING 16
#define MAX_MAC_STRING 18

#define IP_TYPE (u_int16_t)0x0800
#define ARP_TYPE (u_int16_t)0x0806

struct ip_struct {
    unsigned int ip_set;
    u_int32_t ip_addr;
    u_int8_t mask;
};
//node has only ip_strur interface mac and ipstruct

/////////Network-related functions//////////////
void set_ip(struct ip_struct *ip_addr, u_int32_t ip, u_int8_t mask);
void set_mac(u_int8_t *mac_addr, u_int8_t *addr);
u_int32_t get_subnet(u_int32_t ip, u_int8_t mask_bits);
void generate_mac_addr(u_int8_t *mac_addr);
void generate_broadcast_addr(u_int8_t *mac_addr);
int is_broadcast(u_int8_t *mac_addr);
int are_mac_equal(u_int8_t *mac1, u_int8_t *mac2);
char *ip_to_string(char * ip_str, size_t str_lnt, u_int32_t ip);
char *mac_to_string(char * mac_str, size_t str_lnt, u_int8_t *mac);

#endif