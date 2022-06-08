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

};
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
    name_str[INT_NAME_SIZE] = '\0';

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
    graph->topology_name[GR_NAME_SIZE] = '\0';
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

struct graph_node *add_node(struct graph *graph, char *name)
{
    int i;
    struct graph_node *new_graph_node;
    struct doubly_linked_item *new_list_item;

    if(!graph) 
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

    node_net_init(&(new_graph_node->node_net));

    strncpy(new_graph_node->name, name, ND_NAME_SIZE);
    new_graph_node->name[ND_NAME_SIZE] = '\0';
    new_graph_node->socket_fd = -1;
    new_graph_node->socket_port = 0;


    for(i = 0; i < MAX_INTERFACE; i++)
        new_graph_node->interfaces[i] = NULL;

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

    if(to_remove->socket_fd > 0)
        close(to_remove->socket_fd);
    remove_from_list_by_data(graph->nodes, to_remove);
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

    if_net_init(&(interface->if_net));
    generate_mac_addr(&(interface->if_net.mac_addr));
    
    strncpy(interface->name, name, INT_NAME_SIZE);
    interface->name[INT_NAME_SIZE] = '\0';
    
    interface->link = NULL;
    interface->node = node;

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
    if_name[ND_NAME_SIZE] = '\0';

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
    if(!interface)
        return;

    if(interface->link)
        remove_link(interface->link);
    free(interface);   
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

void set_node_ip_addr(struct graph_node *node, u_int32_t ip, u_int8_t mask)
{
    struct ip_addr *ip_addr;

    if(!node)
        return;
    else
        set_ip(&(node->node_net.ip_addr), ip, mask);
}

void set_if_ip_addr(struct interface *interface, u_int32_t ip, u_int8_t mask)
{
    struct ip_addr *ip_addr;

    if(!interface)
        return;
    else
        set_ip(&(interface->if_net.ip_addr), ip, mask);
    
    interface->if_net.ip_addr.mask = mask;
}

struct interface *get_interface_in_subnet(struct graph_node *node, u_int32_t ip)
{
    int i;
    u_int8_t mask;
    u_int32_t subnet0, subnet1;
    struct interface *interface;
    struct if_net net;

    if(!node)
        return NULL;

    for(i = 0; i < MAX_INTERFACE; i++)
    {
        interface = node->interfaces[i];
        if(!interface)
            continue;
        net = interface->if_net;
        if(!net.ip_addr.ip_set)
            continue;
        mask = net.ip_addr.mask;
        subnet0 = get_subnet(net.ip_addr.ip_addr, mask);
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
    name_str[ND_NAME_SIZE] = '\0';

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