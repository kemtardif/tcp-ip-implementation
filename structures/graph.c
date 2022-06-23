#include "graph.h"

struct interface *get_attached_interface(struct interface *interface)
{
    struct graph_link *link;

    if(!interface) 
        return NULL;
    link = interface->link;
    if(!link)
        return NULL;

    if(link->if_1 == interface)
        return link->if_2;
    else
        return link->if_1;

}
int next_available_interface_slot(struct graph_node *node)
{
    int i;
    if(!node) return -1;
    
    for(i = 0; i < MAX_INTERFACE; i++)
    {
        if(!node->interfaces[i])
            return i;
    }
    return -1;
}

struct interface *find_interface_by_name(struct graph_node *node, char *name)
{
    int i;
    struct interface *interface;
    char name_str[INT_NAME_SIZE];

    if(!node || !name) 
        return NULL;
    
    strncpy(name_str, name, INT_NAME_SIZE);
    name_str[INT_NAME_SIZE - 1] = '\0';

    for(i = 0; i < MAX_INTERFACE; i++)
    {
        interface = node->interfaces[i];      
        if(!interface)
           continue;
        else if (!strncmp(interface->name, name_str, INT_NAME_SIZE))
            return interface;
    }
    return NULL;
}

struct graph *graph_init(char *topology_name)
{
    struct graph *graph;   
    if((graph = malloc(sizeof(struct graph))) == NULL)
        return NULL;

    strncpy(graph->topology_name, topology_name, GR_NAME_SIZE - 1);
    graph->topology_name[GR_NAME_SIZE - 1] = '\0';
    if((graph->nodes = malloc(sizeof(struct doubly_linked_list))) == NULL)
    {
        free(graph);
        return NULL;
    }

    graph->nodes->head = NULL;
    graph->node_count = 0;
    graph->is_up = 0;

    return graph;
}

void graph_free(struct graph *graph)
{
    struct graph_node *node, *temp;
    struct doubly_linked_item *item;
    if(!graph) return;

    if(graph->nodes)
    {
        item = graph->nodes->head;
        while(item)
        {
            temp = (struct graph_node *)graph->nodes->head->data;
            item = item->next;         
            if(temp)
                remove_node(graph, temp);
        }
        dll_free(graph->nodes);

    }
    free(graph);
}

struct graph_link *adjacent(struct graph_node *node1, struct graph_node *node2)
{
    int i;
    struct interface *interface;
    struct graph_link *link;

    for(i = 0; i < MAX_INTERFACE; i++)
    {
        interface = node1->interfaces[i];        
        if(!interface) continue;

        link = interface->link;
        if(link->if_1->node == node2 
            || link->if_2->node == node2)
            return link;
    }   
    return NULL;
}

struct graph_node *add_node(struct graph *graph, char *name, int type)
{
    int i;
    struct graph_node *new_graph_node;
    struct doubly_linked_item *new_list_item;

    if(!graph || !name || !type) 
        return NULL;
    if(graph->node_count == MAX_NODE)
        return NULL;
    if((new_graph_node = find_node_by_name(graph, name)) != NULL)
        return NULL;       
    if((new_graph_node = malloc(sizeof(struct graph_node))) == NULL) 
        return NULL;
    if((new_list_item = malloc(sizeof(struct doubly_linked_item))) == NULL)
    {
        free(new_graph_node);
        return NULL;
    }

    strncpy(new_graph_node->name, name, ND_NAME_SIZE);
    new_graph_node->name[ND_NAME_SIZE - 1] = '\0';
    new_graph_node->type = type;


    for(i = 0; i < MAX_INTERFACE; i++)
        new_graph_node->interfaces[i] = NULL;

    new_graph_node->switch_table = NULL;
    if(type == SWITCH)
        new_graph_node->switch_table = init_hash_table(SUBNET_CAPACITY);


    new_list_item->data = new_graph_node;
    add_to_list(graph->nodes, new_list_item);

    graph->node_count++;

    return new_graph_node;
}

void remove_node(struct graph *graph, struct graph_node *to_remove)
{
    int i;
    struct interface *interface;
    struct graph_link *link;
    if(!graph || !to_remove)
        return;

    for(i = 0; i < MAX_INTERFACE; i ++)
        free_interface(to_remove->interfaces[i]);
    remove_from_list_by_data(graph->nodes, to_remove);
    if(to_remove->type == SWITCH)
        free_hash_table(to_remove->switch_table);
    free(to_remove);

    graph->node_count--;
}

struct interface *add_interface(struct graph_node *node, char *name)
{
    int i;
    struct interface *interface;

    if(!node)
     return NULL;
    //Unique interface name in each node
    if((interface = find_interface_by_name(node, name)) != NULL)
        return NULL;
    //No available NIC slot in node
    if((i = next_available_interface_slot(node)) == -1)
        return NULL;
    interface = add_interface_at_index(node, i, name);
    return interface;
}

struct interface  *add_interface_at_index(struct graph_node *node, unsigned int index, char *name)
{
    struct interface *interface;

    if(!node)
     return NULL;

    //Slot taken
    if(node->interfaces[index])
        return NULL;

    if((interface = malloc(sizeof(struct interface))) == NULL)
        return NULL;
    if((interface->arp_table = init_hash_table(SUBNET_CAPACITY)) == NULL)
    {
        free_interface(interface);
        return NULL;
    }
    if((interface->send_queue = init_queue(MAX_QUEUE)) == NULL)
    {
        free(interface->arp_table);
        free(interface);
        return NULL;
    }
    
    if_net_init(interface);
    generate_mac_addr(interface->mac_addr);
    
    strncpy(interface->name, name, INT_NAME_SIZE);
    interface->name[INT_NAME_SIZE - 1] = '\0';
    
    interface->link = NULL;
    interface->node = node;
    interface->port = 0;

    interface->arp_table = NULL;
    if(interface->node->type != SWITCH)
        interface->arp_table = init_hash_table(SUBNET_CAPACITY);                

    node->interfaces[index] = interface;
    return interface;
}

void remove_interface_by_name(struct graph_node *node, char *name)
{
    int i;
    char if_name[INT_NAME_SIZE];
    struct graph_node *_node = node;
    struct interface *itf;

    if(!node)
        return;

    strncpy(if_name, name, INT_NAME_SIZE);
    if_name[ND_NAME_SIZE - 1] = '\0';

    for(i = 0; i < MAX_INTERFACE; i++)
    {
        itf = _node->interfaces[i];
        if(!itf)
            continue;
        if(!strncmp(itf->name, if_name, strlen(itf->name)))
        {   
            free_interface(itf);
            _node->interfaces[i] = NULL;
            return;
        }
    }
}   

void free_interface(struct interface *interface)
{
    struct send_packet *packet;
    if(!interface)
        return;
    if(interface->link)
        remove_link(interface->link);
    if(interface->node->type != SWITCH)
        free_hash_table(interface->arp_table);
    while((packet = (struct send_packet *)pop(interface->send_queue)) != NULL)
    {
        free(packet->packet);
        free(packet);
    }
    free_queue(interface->send_queue);
    free(interface);   
}

struct interface *find_src_interface_by_src_mac(struct graph_node *node, u_int8_t *mac_addr_src)
{
    int i;
    struct interface *itf, *srcItf;
    if(!node)
        return NULL;

    for(i = 0; i < MAX_INTERFACE; i++)
    {
        itf = node->interfaces[i];
        if(!itf)
         continue;
        srcItf = get_attached_interface(itf);
        if(!srcItf)
            continue;
        if(are_mac_equal(srcItf->mac_addr, mac_addr_src))
            return srcItf;
    }
    return NULL;
}

struct interface *find_dest_interface_by_src_mac(struct graph_node *node, u_int8_t *mac_addr_src)
{
    struct interface *itf;
    if(!node || !mac_addr_src)
        return NULL;
     //Find source interface from eth frame
    if((itf = find_src_interface_by_src_mac(node, mac_addr_src)) == NULL)
        return NULL;;
    return get_attached_interface(itf);    
}

struct graph_link  *add_link(struct interface *if1,
                             struct interface *if2,
                             unsigned int cost)
{
    struct graph_link *link;

    if(!if1 || !if2 || if1 == if2) 
        return NULL;
    if(if1->link || if2->link)
        return NULL;
    if((link = malloc(sizeof(struct graph_link))) == NULL) 
        return NULL;

    link->if_1 = if1;
    link->if_2 = if2;
    link->cost = cost;

    if1->link = link;
    if2->link = link;

    return link;
}

void remove_link(struct graph_link *link)
{
    if(!link) 
        return;
    //Disconnonnect attached interface;
    link->if_1->link = NULL;
    link->if_2->link = NULL;

    free(link);
}

void set_itf_ip(struct interface *interface, u_int32_t ip, u_int8_t mask)
{
    if(!interface)
        return;
    else
        set_ip(&(interface->ip), ip, mask);
    
    interface->ip.mask = mask;
}

struct interface *get_interface_in_subnet(struct graph_node *node, u_int32_t ip)
{
    int i;
    u_int8_t mask;
    u_int32_t subnet0, subnet1;
    struct interface *interface;
    struct ip_struct net;

    if(!node)
        return NULL;

    for(i = 0; i < MAX_INTERFACE; i++)
    {
        interface = node->interfaces[i];
        if(!interface)
            continue;
        net = interface->ip;
        if(!net.ip_set)
            continue;
        mask = net.mask;
        subnet0 = get_subnet(net.ip_addr, mask);
        subnet1 = get_subnet(ip, mask);

        if(!(subnet0 ^ subnet1))
            return interface;
    }
    return NULL;
}

struct graph_node *find_node_by_name(struct graph *graph, char *name)
{
    struct doubly_linked_list *items;
    struct doubly_linked_item *item;
    struct graph_node *graph_node;
    char name_str[ND_NAME_SIZE];

    if(!graph) 
        return NULL;

    items = graph->nodes;
    if(!items) 
        return NULL;

    strncpy(name_str, name, ND_NAME_SIZE);
    name_str[ND_NAME_SIZE - 1] = '\0';

    item = items->head;
    while(item)
    {
        graph_node = (struct graph_node *)item->data;

        if(!strncmp(graph_node->name, name_str, strlen(graph_node->name)))
            return graph_node;

        item = item->next;
    }
    return NULL;
}

void if_net_init(struct interface *itf)
{
    int i;
    itf->ip.ip_set = 0;
    itf->ip.ip_addr = 0x0;
    itf->ip.mask = 32;

    for(i = 0; i < MAC_SIZE; i++)
       itf->mac_addr[i] = 0;
}