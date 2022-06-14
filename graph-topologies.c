#include "structures/graph.h"



/*
This is a simple topology with 3 hosts LAN connected by a single switch. 
*/
struct graph *three_devices_topology()
{

    struct graph *graph;
    struct graph_node *R0_re, *R1_re, *R2_re, *R3_swtch;
    struct interface *if0, *if1, *if2, *if3, *if4, *if5;
    struct graph_link *l0, *l1, *l2;

    //Create Network
    if((graph = graph_init("3-devices network")) == NULL)
        return NULL;

    //Add the three devices
    if((R0_re = add_node(graph, "R0_hst", HOST)) == NULL
         || (R1_re = add_node(graph, "R1_hst", HOST)) == NULL 
         || (R2_re = add_node(graph, "R2_hst", HOST)) == NULL
         || (R3_swtch = add_node(graph, "R3_swtch", SWITCH)) == NULL)
    {
        graph_free(graph);
        return NULL;
    }
    //Create and attach all interfaces
    if((if0 = add_interface(R0_re, "eth/0")) == NULL
        || (if1 = add_interface(R1_re, "eth/1")) == NULL
        || (if2 = add_interface(R2_re, "eth/2")) == NULL
        || (if3 = add_interface(R3_swtch, "eth/3")) == NULL
        || (if4 = add_interface(R3_swtch, "eth/4")) == NULL
        || (if5 = add_interface(R3_swtch, "eth/5")) == NULL)
    {
        graph_free(graph);
        return NULL;
    }

    set_itf_ip(if0, (u_int32_t)1869573890, 24); 
    set_itf_ip(if1, (u_int32_t)1869573891, 24);
    set_itf_ip(if2, (u_int32_t)1869573892, 24);

    //Add links between the interfaces;
    if((l0 = add_link(if0, if3, 1)) == NULL
        || (l1 = add_link(if1, if4, 1)) == NULL
        || (l2 = add_link(if2, if5, 1)) == NULL)
    {
        graph_free(graph);
        return NULL;
    }

    return graph;
}

