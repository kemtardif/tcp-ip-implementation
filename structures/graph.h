#ifndef _GRAPH_H_
#define _GRAPH_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "dll.h"
#include "net.h"
#include "hash.h"
#include "queue.h"

#define GR_NAME_SIZE 32
#define ND_NAME_SIZE 16
#define MAX_INTERFACE 10
#define INT_NAME_SIZE 16
#define MAX_NODE 100
#define MAX_QUEUE 8
#define ARP_BUFFER_LENGTH 4

//Types of nodes
#define UNDEFINED 0
#define HOST 1
#define ROUTER 2
#define SWITCH 3

//Wait type
#define NO_WAIT 0
#define ARP_WAIT 1

//send type
#define READY 1
#define WAIT

////////////////Structure definitions////////////////////////////

//This would be the actual network, with a list of network devices
struct graph {
    char topology_name[GR_NAME_SIZE];
    int node_count;
    unsigned long is_up;
    struct doubly_linked_list *nodes;
};

/*
This  would be a networking device with LAN interfaces.
In this case, a NULL interface is just an empty slot.
Type field determine if its either a host, switch or router.
*/
struct graph_node {
    char name[ND_NAME_SIZE];
    int type;
    struct hash_table *switch_table;
    struct interface *interfaces[MAX_INTERFACE];
};

struct arp_buffer_item
{
    u_int32_t target_ip;
    char *packet;
    size_t pckt_size;
};

/*This would be a LAN interface, connected to a network device (graph_node)
and connecting to another node via a physical link (link).If if_net struct
is set, it is a router, otherwise it is a switch.
*/
struct interface {
    char name[INT_NAME_SIZE];
    u_int8_t mac_addr[MAC_SIZE];
    struct ip_struct ip;
    unsigned int port;
    struct graph_node *node;
    struct graph_link *link;
    struct hash_table *arp_table;
    struct arp_buffer_item arp_buffer[ARP_BUFFER_LENGTH];
    struct queue *send_queue;
};
/*Those are packet ready to be sent on an interface
*/
struct send_packet
{
    char *packet;
    size_t pckt_size;
    int retransmit;
};

//This would be a physical link connecting two LAN interfaces
struct graph_link 
{
    struct interface *if_1;
    struct interface *if_2;
    unsigned int cost;
};




////////////////Functions///////////////////////////////

struct graph *graph_init(char *topology_name);
//Free whole graph and attached structures (dll, nodes and links)
void graph_free(struct graph *graph);
//Return a pointer to a link between nodes if it exist, otherwise return NULL
struct graph_link *adjacent(struct graph_node *node1, struct graph_node *node2);
//Return added node if it is not in graph, otherwise return NULL
struct graph_node *add_node(struct graph *graph, char *name, int type);
//Remove and free node, its associated interfaces and all connected links
void remove_node(struct graph *graph, struct graph_node *to_remove);
//Attach an a new interface at first available slot. Return NULL otherwise
struct interface *add_interface(struct graph_node *node, char *name);
//Return new interface if attached, otherwise NULL
struct interface  *add_interface_at_index(struct graph_node *node, 
                                          unsigned int index, 
                                          char *name);
//Remove interface by name if itexists
void remove_interface_by_name(struct graph_node *node, char *name);
//Return created link, otherwise NULL
struct graph_link *add_link(struct interface *if1,
                            struct interface *if2,
                            unsigned int cost);
//Remove and free link struct and it's dll item in graph.
void remove_link(struct graph_link *link);

//////////////Networking functions on structures/////////////////
void set_itf_ip(struct interface *interface, u_int32_t ip, u_int8_t mask);
struct interface *get_interface_in_subnet(struct graph_node *node, u_int32_t ip);

///////////////////Helper functions////////////////////////////

//Return node to which the interface is connected by a link
struct interface *get_attached_interface(struct interface *interface);
//Return NULL if not found
struct graph_node *find_node_by_name(struct graph *graph, char *name);
//Return index of next available interface slot
int next_available_interface_slot(struct graph_node *node);
//Return interface by name, or NULL otherwise
struct interface *find_interface_by_name(struct graph_node *node, char *name);
//Free interface and associated link, if any.
void free_interface(struct interface *interface);
struct interface *find_src_interface_by_src_mac(struct graph_node *node, u_int8_t *mac_addr_src);
struct interface *find_dest_interface_by_src_mac(struct graph_node *node, u_int8_t *mac_addr_src);
void node_net_init(struct ip_struct *ip);
void if_net_init(struct interface *itf);
#endif 

