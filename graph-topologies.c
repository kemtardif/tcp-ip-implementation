#include <stdlib.h>
#include <sys/types.h>
#include "structures/net.h"
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

    set_node_ip_addr(R0_re, (u_int32_t)2394452033);
    set_node_ip_addr(R1_re, (u_int32_t)1085738960);
    set_node_ip_addr(R2_re, (u_int32_t)9675987479);

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

    set_if_ip_addr(if0, (u_int32_t)1584638609, 32);
    set_if_ip_addr(if1, (u_int32_t)9364857802, 24);
    set_if_ip_addr(if2, (u_int32_t)1537935793, 32);
    set_if_ip_addr(if3, (u_int32_t)1856479437, 24);
    set_if_ip_addr(if4, (u_int32_t)2547693578, 32);
    set_if_ip_addr(if5, (u_int32_t)1857237567, 24);

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

