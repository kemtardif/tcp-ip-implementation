#include <stdlib.h>
#include "structures/graph.h"



/*
This is a simple 3 devices topology. Each device has 2 LAN adapters,
with each link connected to one of the neighbor, hence forming
a "closed circle".
*/
struct graph *three_devices_topology()
{

    struct graph *graph;
    struct graph_node *R0_re, *R1_re, *R2_re;
    struct interface *if0, *if1, *if2, *if3, *if4, *if5;
    struct graph_link *l0, *l1, *l2;

    //Create Network
    if((graph = graph_init("3-devices network")) == NULL)
        return NULL;

    //Add the three devices
    if((R0_re = add_node(graph, "R0_re")) == NULL
         || (R1_re = add_node(graph, "R1_re")) == NULL 
         || (R2_re = add_node(graph, "R2_re")) == NULL)
    {
        graph_free(graph);
        return NULL;
    }

    //Create and attach all interfaces
    if((if0 = add_interface(R0_re, "eth/0")) == NULL
        || (if1 = add_interface(R0_re, "eth/1")) == NULL
        || (if2 = add_interface(R1_re, "eth/2")) == NULL
        || (if3 = add_interface(R1_re, "eth/3")) == NULL
        || (if4 = add_interface(R2_re, "eth/4")) == NULL
        || (if5 = add_interface(R2_re, "eth/5")) == NULL)
    {
        graph_free(graph);
        return NULL;
    }

    //Add links between the interfaces;
    if((l0 = add_link(if1, if2, 1)) == NULL
        || (l1 = add_link(if3, if4, 1)) == NULL
        || (l2 = add_link(if5, if0, 1)) == NULL)
    {
        graph_free(graph);
        return NULL;
    }

    return graph;
}

