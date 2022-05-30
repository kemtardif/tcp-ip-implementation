#ifndef _GRAPH_H_
#define _GRAPH_H_

#define GR_NAME_SIZE 32
#define ND_NAME_SIZE 16
#define MAX_INTERFACE 10
#define INT_NAME_SIZE 16

////////////////Structure definitions////////////////////////////

//This would be the actual network, with a list of network devices
struct graph {
    char topology_name[GR_NAME_SIZE];
    struct doubly_linked_list *nodes;
};

/*
This  would be a networking device with LAN interfaces.
In this case, a NULL interface is just an empty slot.
*/
struct graph_node {
    char name[ND_NAME_SIZE];
    struct node_net node_net;
    struct interface *interfaces[MAX_INTERFACE];
};

/*This would be a LAN interface, connected to a network device (graph_node)
and connecting to another node via a physical link (link)
*/
struct interface {
    char name[INT_NAME_SIZE];
    struct if_net if_net;
    struct graph_node *node;
    struct graph_link *link;
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
void graph_free(struct graph *graph);
//Return a pointer to a link between nodes if it exist, otherwise return NULL
struct graph_link *adjacent(struct graph_node *node1, struct graph_node *node2);
//Return added node if it is not in graph, otherwise return NULL
struct graph_node *add_node(struct graph *graph, char *name);
//Remove and free node, its associated interfaces and all connected links
void remove_node(struct graph *graph, struct graph_node *to_remove);
//Attach an a new interface at first available slot. Return NULL otherwise
struct interface *add_interface(struct graph_node *node, char *name);
//Return new interface if attached, otherwise NULL
struct interface  *add_interface_at_index(struct graph_node *node, 
                                          unsigned int index, 
                                          char *name);
//Remove interface and associated link, if it exists
void remove_interface(struct interface *interface);
//Return created link, otherwise NULL
struct graph_link *add_link(struct interface *if1,
                            struct interface *if2,
                            unsigned int cost);
//Remove and free link struct and it's dll item in graph.
void remove_link(struct graph_link *link);

//////////////Networking functions on structures/////////////////
void set_node_ip_addr(struct graph_node *node, u_int32_t ip);
void set_if_ip_addr(struct interface *interface, u_int32_t ip, u_int8_t mask);
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
//This print all the available graph informations and configuration
void print_graph(struct graph *graph);
#endif 

