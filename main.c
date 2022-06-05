#include <libcli.h>
#include <netinet/in.h>
#include <string.h>
#include <limits.h>
#include <arpa/inet.h>
 #include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "structures/graph.h"
#include "config.h"


#define PORT 8000
#define NAME "name"
#define NAME_IF "name_2"
#define COST "cost"
#define IP "ip"
#define SUBNET "subnet"


struct context
{
    int is_topo_set;
    struct graph *graph;
};

void run(int fd);
int cmd_add_graph(struct cli_def *cli, const char *command, char *argv[], int argc);
int cmd_add_node(struct cli_def *cli, const char *command, char *argv[], int argc);
int cmd_add_interface(struct cli_def *cli, const char *command, char *argv[], int argc);
int cmd_add_link(struct cli_def *cli, const char *command, char *argv[], int argc);
int cmd_remove_network(struct cli_def *cli, const char *command, char *argv[], int argc);
int cmd_remove_node(struct cli_def *cli, const char *command, char *argv[], int argc);
int cmd_remove_interface(struct cli_def *cli, const char *command, char *argv[], int argc);
int cmd_remove_link(struct cli_def *cli, const char *command, char *argv[], int argc);
int cmd_set_node(struct cli_def *cli, const char *command, char *argv[], int argc);
int cmd_set_interface(struct cli_def *cli, const char *command, char *argv[], int argc);
int cmd_dump(struct cli_def *cli, const char *command, char *argv[], int argc);
int cmd_show_node(struct cli_def *cli, const char *command, char *argv[], int argc);
int graph4graph_validator(struct cli_def *cli, const char *name, const char *value);
int graph_validator(struct cli_def *cli, const char *name, const char *value);
int value_validator(struct cli_def *cli, const char *name, const char *value);
int node_name_validator(struct cli_def *cli, const char *name, const char *value);
int is_graph_set(struct cli_def *cli);
struct graph *get_current_graph(struct cli_def *cli);
void print_graph(struct cli_def *cli, struct graph *graph);
void print_node(struct cli_def *cli, struct graph_node *node);
void print_ip(struct cli_def *cli, struct ip_addr *ip_addr);
void print_subnet(struct cli_def *cli, struct ip_addr *ip_addr);
void print_mac(struct cli_def *cli, struct mac_addr *mac_addr);
int set_ip_c(struct cli_def *cli, struct ip_addr *ip_addr, char *value);

int main()
{
    struct sockaddr_in addr;
    int x, s, opt = 1;

    //Create socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(int));

    // Listen on port 8000
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    bind(s, (struct sockaddr *)&addr, sizeof(addr));

    // Wait for a connection
    if(listen(s, 50) == -1)
    {
        printf("* could not listen. Terminate. \n");
        return -1;
    }

    while ((x = accept(s, NULL, 0))) {
        run(x);     
        close(x);
    }
  return 0;
}

void run(int fd)
{
    struct cli_def *cli;
    struct cli_command *c, *d;
    struct cli_optarg *o;
    struct context cntx;
    size_t i;


    cli = cli_init();
    cli_set_banner(cli, "TCP/IP stack networking CLI");
    cli_set_hostname(cli, "network");

    //Add _
    c = cli_register_command(cli, NULL, "add", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, 
                                "Add the various structures");
    //_graph
    d = cli_register_command(cli, c, "network", cmd_add_graph, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, 
                                "Add network graph");
    o = cli_register_optarg(d, NAME, CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify a new or configured network by name", NULL, graph4graph_validator, NULL);
    //Pre-configured networks
    cli_optarg_addhelp(o, "3-devices", "Simple, circular 3-devices network");
    //_node
    d = cli_register_command(cli, c, "node", cmd_add_node, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, 
                                "Add network node");
    o = cli_register_optarg(d, NAME, CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify a node name", NULL, graph_validator, NULL);
    //_interface
    d = cli_register_command(cli, c, "interface", cmd_add_interface, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, 
                                "Add network interface to node");
    o = cli_register_optarg(d, NAME, CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify a node name", NULL, graph_validator, NULL);
    o = cli_register_optarg(d, NAME_IF, CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify an interface name", NULL, value_validator, NULL);
    //_link
    d = cli_register_command(cli, c, "link", cmd_add_link, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, 
                                "Add link between to free interface");
    o = cli_register_optarg(d, "node_1", CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify a node 1", NULL, graph_validator, NULL);
    o = cli_register_optarg(d, "node_2", CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify node 2", NULL, value_validator, NULL);
    o = cli_register_optarg(d, "if_1", CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify interface 1", NULL, value_validator, NULL);
    o = cli_register_optarg(d, "if_2", CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify interface 2", NULL, value_validator, NULL);
    o = cli_register_optarg(d, "cost", CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify a link cost", NULL, value_validator, NULL);

    //Remove _
    c = cli_register_command(cli, NULL, "remove", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    //_graph
    d = cli_register_command(cli, c, "network", cmd_remove_network , PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Release current network");
    //_node
    d = cli_register_command(cli, c, "node", cmd_remove_node, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Release a node");
    o = cli_register_optarg(d, NAME, CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify a node name", NULL, graph_validator, NULL);
    //_interface
    d = cli_register_command(cli, c, "interface", cmd_remove_interface, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Release an interface");
    o = cli_register_optarg(d, NAME, CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify a node name", NULL, graph_validator, NULL);
    o = cli_register_optarg(d, NAME_IF, CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify an interface name", NULL, value_validator, NULL);
    //_link
    d = cli_register_command(cli, c, "link", cmd_remove_link, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, 
                                "Remove link on interface");
    o = cli_register_optarg(d, "node", CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify a node", NULL, graph_validator, NULL);
    o = cli_register_optarg(d, "interface", CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify an interface", NULL, value_validator, NULL);

    //Set _
    c = cli_register_command(cli, NULL, "set", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    //_node
    d = cli_register_command(cli, c, "node", cmd_set_node, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Set node values");
    o = cli_register_optarg(d, "name", CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify a node name", NULL, graph_validator, NULL);
    o = cli_register_optarg(d, "ip", CLI_CMD_OPTIONAL_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Set ip address/mask in format 255.255.255.255/32", NULL, value_validator, NULL);
    //
    d = cli_register_command(cli, c, "interface", cmd_set_interface, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Set interface values");
    o = cli_register_optarg(d, "node_name", CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify a node", NULL, graph_validator, NULL);
    o = cli_register_optarg(d, "interface_name", CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify an interface", NULL, value_validator, NULL);
    o = cli_register_optarg(d, "ip", CLI_CMD_OPTIONAL_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Set ip address/mask in format 255.255.255.255/32", NULL, value_validator, NULL);
    
    //Dump network informations
    cli_register_command(cli, NULL, "dump", cmd_dump, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Dump current network informations");

    //print structures
    c = cli_register_command(cli, NULL, "show", NULL, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL);
    c = cli_register_command(cli, c, "node", cmd_show_node , PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Print node informations.");
    o = cli_register_optarg(c, NAME, CLI_CMD_ARGUMENT, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Specify a node by name.", NULL, graph_validator, NULL);
    o = cli_register_optarg(c, IP, CLI_CMD_HYPHENATED_OPTION, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Get ip address", NULL, value_validator, NULL);
    o = cli_register_optarg(c, SUBNET, CLI_CMD_HYPHENATED_OPTION, PRIVILEGE_UNPRIVILEGED, MODE_EXEC, "Get subnet", NULL, value_validator, NULL);

    cntx.is_topo_set = 0;
    cntx.graph = NULL;
    cli_set_context(cli, (void *)&cntx);

    cli_loop(cli, fd);
    cli_done(cli);
}

int cmd_add_graph(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    struct graph *graph = NULL;
    struct context *cntx;
    char *name = cli_get_optarg_value(cli, NAME, NULL);

    if(!strncmp(name, "new", strlen("new")))
    {
        if(!argc)
        {
            cli_error(cli, "Must provide a valid name for network./n");
            return CLI_ERROR;
        }
        graph = graph_init(argv[0]);
    }
    else if(!strncmp(name, "3-devices", strlen("3-devices")))
        graph = three_devices_topology();

    if(!graph)
    {
        cli_error(cli, "\nCritical error while creating network.\n");
        return CLI_ERROR;
    }

    cntx = (struct context *)cli_get_context(cli);
    cntx->is_topo_set = 1;
    cntx->graph = graph;
    cli_print(cli, "\nNetwork %s is initialized.\n", graph->topology_name);
    return CLI_OK;

}

int cmd_add_node(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    struct graph *graph = get_current_graph(cli);
    struct graph_node *node;
    char *name = cli_get_optarg_value(cli, NAME, NULL); 

    if((node = add_node(graph, name)) == NULL)
    {
        cli_error(cli, "Could not add node %s to network.\n", name);
        return CLI_ERROR;
    } 
    cli_print(cli, "Node %s added.\n", name);
    return CLI_OK;
}

int cmd_add_interface(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    char *name = cli_get_optarg_value(cli, NAME, NULL);
    char *name_if = cli_get_optarg_value(cli, NAME_IF, NULL);
    struct graph *graph = get_current_graph(cli);
    struct graph_node *node = find_node_by_name(graph, name);
    struct interface *itf;

    if(!node)
    {
        cli_error(cli, "Node %s not in network.\n", name);
        return CLI_ERROR;
    }

    if((itf = add_interface(node, name_if)) == NULL)
    {
        cli_error(cli, "Could not add interface %s to node.\n", name_if);
        return CLI_ERROR;
    }

    cli_print(cli, "Interface %s added to node.\n", name_if);
    return CLI_OK;
}

int cmd_add_link(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    struct graph *graph = get_current_graph(cli);
    struct graph_node *node_1, *node_2;
    struct interface *if_1, *if_2;
    struct graph_link *link;
    char *noden_1, *noden_2, *ifn_1, *ifn_2;
    char *cost_n = cli_get_optarg_value(cli, "cost", NULL);
    char *ptr;
    unsigned long cost_l = strtoul(cost_n, &ptr, 10);
    unsigned int cost;
    printf("BEFORE COST\n");
    if(*ptr != '\0' || cost_l > UINT_MAX)
    {
        cli_error(cli, "Invalid cost value.\n");
        return CLI_ERROR;
    }
    cost = (unsigned int)cost_l;

    noden_1 = cli_get_optarg_value(cli, "node_1", NULL);
    if((node_1 = find_node_by_name(graph, noden_1)) == NULL)
    {
        cli_error(cli, "Node %s not in network.\n", noden_1);
        return CLI_ERROR;
    }

    noden_2 = cli_get_optarg_value(cli, "node_2", NULL);
    if((node_2 = find_node_by_name(graph, noden_2)) == NULL)
    {
        cli_error(cli, "Node %s not in network.\n", noden_2);
        return CLI_ERROR;
    }

    ifn_1 = cli_get_optarg_value(cli, "if_1", NULL);
    if((if_1 = find_interface_by_name(node_1, ifn_1)) == NULL)
    {
        cli_error(cli, "Interface %s not attached to node %s.\n", ifn_1, noden_1);
        return CLI_ERROR;
    }

    ifn_2 = cli_get_optarg_value(cli, "if_2", NULL);
    if((if_2 = find_interface_by_name(node_2, ifn_2)) == NULL)
    {
        cli_error(cli, "Interface %s not attached to node %s.\n", ifn_2, noden_2);
        return CLI_ERROR;
    }

    if((link = add_link(if_1, if_2, cost)) == NULL)
    {
        cli_error(cli, "Could not attach nodes\n");
        return CLI_ERROR;
    }

    cli_print(cli, "Nodes are attached.\n");
    return CLI_OK;
}


int cmd_remove_network(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    struct graph *graph = get_current_graph(cli);
    struct context *cntx;

    if(!graph)
    {
        cli_error(cli, "\nNo network to release.\n");
        return CLI_ERROR;
    }
    graph_free(graph);

    cntx = (struct context *)cli_get_context(cli);
    cntx->is_topo_set = 0;
    cntx->graph = NULL;
    cli_print(cli, "\nNetwork released.\n");
    return CLI_OK;
}

int cmd_remove_node(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    char *name = cli_get_optarg_value(cli, NAME, NULL);
    struct graph *graph = get_current_graph(cli);
    struct graph_node *node = find_node_by_name(graph, name);

    if(!node)
    {
        cli_error(cli, "Node %s not in network.\n", name);
        return CLI_ERROR;
    }
    remove_node(graph, node);
    cli_print(cli, "Node %s removed.\n", name);
    return CLI_OK;
}

int cmd_remove_interface(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    char *name = cli_get_optarg_value(cli, NAME, NULL);
    char *name_if = cli_get_optarg_value(cli, NAME_IF, NULL);
    struct graph *graph = get_current_graph(cli);
    struct graph_node *node = find_node_by_name(graph, name);
    struct interface *itf;

    if(!node)
    {
        cli_error(cli, "Node %s not in network.\n", name);
        return CLI_ERROR;
    }

    remove_interface_by_name(node, name_if);
    cli_print(cli, "Interface %s removed from node.\n", name_if);
    return CLI_OK;

}

int cmd_remove_link(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    char *name_n = cli_get_optarg_value(cli, "node", NULL);
    char *name_if = cli_get_optarg_value(cli, "interface", NULL);
    struct graph *graph = get_current_graph(cli);
    struct graph_node *node;
    struct graph_link *link;
    struct interface *interface;

    if((node = find_node_by_name(graph, name_n)) == NULL)
    {
        cli_error(cli, "Node %s not in network.\n", name_n);
        return CLI_ERROR;
    }
    if((interface = find_interface_by_name(node, name_if)) == NULL)
    {
        cli_error(cli, "Interface %s not on node.\n", name_if);
        return CLI_ERROR;
    }
    if((link = interface->link) == NULL)
    {
        cli_error(cli, "Interface %s is not linked.\n", name_if);
        return CLI_ERROR;
    }
    remove_link(link);
    cli_print(cli, "Link removed from interface.\n");
    return CLI_OK;

}

int cmd_set_node(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    struct graph *graph = get_current_graph(cli);
    struct graph_node *node;
    char *n_name, *value;

    n_name = cli_get_optarg_value(cli, "name", NULL);
    if((node = find_node_by_name(graph, n_name)) == NULL)
    {
        cli_error(cli, "\nNode not in network.\n");
        return CLI_ERROR;
    }

    if(((value = cli_get_optarg_value(cli, "ip", NULL)) != NULL)
        && !set_ip_c(cli, &(node->node_net.ip_addr), value))
            return CLI_ERROR;
    else
    {
        cli_error(cli, "\nNo valid parameter to set.\n");
        return CLI_ERROR;
    }
    return CLI_OK;
}

int cmd_set_interface(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    struct graph *graph = get_current_graph(cli);
    struct graph_node *node;
    struct interface *itf;
    char *n_name, *if_name, *value;

    n_name = cli_get_optarg_value(cli, "node_name", NULL);
    if((node = find_node_by_name(graph, n_name)) == NULL)
    {
        cli_error(cli, "\nNode not in network.\n");
        return CLI_ERROR;
    }

    if_name = cli_get_optarg_value(cli, "interface_name", NULL);
    if((itf = find_interface_by_name(node, if_name)) == NULL)
    {
        cli_error(cli, "\nInterface not on node.\n");
        return CLI_ERROR;
    }

    if(((value = cli_get_optarg_value(cli, "ip", NULL)) != NULL)
        && !set_ip_c(cli, &(itf->if_net.ip_addr), value))
            return CLI_ERROR;
    else
    {
        cli_error(cli, "\nNo valid parameter to set.\n");
        return CLI_ERROR;
    }
    return CLI_OK;
}

int cmd_dump(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    struct graph *graph = get_current_graph(cli);

    if(!graph)
    {
        cli_error(cli, "\nNo network to dump.\n");
        return CLI_ERROR;
    }
    print_graph(cli, graph);
    return CLI_OK;
}
int cmd_show_node(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    struct graph *graph = get_current_graph(cli);
    struct graph_node*node;
    struct node_net net;
    char *arg, *name = cli_get_optarg_value(cli, name, NULL);
    int set = 0;
    node = find_node_by_name(graph, name);
    if(!node)
    {
        cli_error(cli, "Node not in network.\n");
        return CLI_ERROR;
    }

    net = node->node_net;
    if((arg = cli_get_optarg_value(cli, IP, NULL)) != NULL)
    {
        print_ip(cli, &(net.ip_addr));
        set = 1;
    }
    if((arg = cli_get_optarg_value(cli, SUBNET, NULL)) != NULL)
    {
        print_subnet(cli, &(net.ip_addr));
        set = 1;
    }
    if(!set)
        print_node(cli, node);

    return CLI_OK;
}

int graph4graph_validator(struct cli_def *cli, const char *name, const char *value) 
{
    if(is_graph_set(cli))
    {
        cli_error(cli, "\nNetwork already initialized.\n");
        return CLI_ERROR;
    }

    cli_set_optarg_value(cli, name, value, 0);
    return CLI_OK;
}

int graph_validator(struct cli_def *cli, const char *name, const char *value)
{
    if(!is_graph_set(cli))
    {
        cli_error(cli, "\nNo network initialized.\n");
        return CLI_ERROR;
    }

    cli_set_optarg_value(cli, name, value, 0);
    return CLI_OK;
}

int value_validator(struct cli_def *cli, const char *name, const char *value)
{
    cli_set_optarg_value(cli, name, value, 0);
    return CLI_OK;
}

int is_graph_set(struct cli_def *cli)
{
    struct context *cntx;

    if(!cli)
        return 0;
        
    cntx = (struct context *)cli_get_context(cli); 
    if(cntx && cntx->is_topo_set)
        return 1;
    else
        return 0;
}

struct graph *get_current_graph(struct cli_def *cli)
{
    struct context *cntx;
    struct graph *graph;

    if(!cli)
        return NULL;

    cntx = (struct context *)cli_get_context(cli); 
    if(cntx && cntx->is_topo_set)
        return cntx->graph;
    else
     return NULL;
}

void print_graph(struct cli_def *cli, struct graph *graph)
{
    int i;
    struct doubly_linked_item *item;
    struct graph_node *node;
    struct interface *interface;
    struct graph_link* link;
    if(!graph)
    {
        printf("Graph is NULL. END\n");
        return;
    }
    //Print name of graph
    cli_print(cli, "Network : %s.\n", graph->topology_name);

    //Enumerate all the nodes and their interfaces
    item = graph->nodes->head;

    if(!item)
    {
        cli_print(cli, "\nGraph has no node.\n");
        return;
    }

    while(item)
    {
        node = (struct graph_node *)item->data;

        cli_print(cli, "***************************************\n");
        print_node(cli, node);
        cli_print(cli, "***************************************\n");
        for(i = 0; i < MAX_INTERFACE; i++)
        {
            interface = node->interfaces[i];
            if(!interface)
                continue;
            cli_print(cli, "Interface : %s\n", interface->name);
            print_mac(cli, &(interface->if_net.mac_addr));
            print_ip(cli, &(interface->if_net.ip_addr));
            print_subnet(cli, &(interface->if_net.ip_addr));
            
            interface = get_attached_interface(interface);
            if(!interface)
                cli_print(cli, "Interface is free\n");
            else
                cli_print(cli, "Linked to : %s\n",  interface->name);
            
            if(i != MAX_INTERFACE - 1)
                cli_print(cli, "          -----------              \n");
        }
        cli_print(cli, "+++++++++++++++++++++++++++++++++++++++\n");
        cli_print(cli, "***************************************\n");

        item = item->next;
    }
}

void print_node(struct cli_def *cli, struct graph_node *node)
{
    cli_print(cli, "Node : %s\n", node->name);
    print_ip(cli, &(node->node_net.ip_addr));
    print_subnet(cli, &(node->node_net.ip_addr));
}

void print_ip(struct cli_def *cli, struct ip_addr *ip_addr)
{
    int i;
    u_int8_t parts[IP_SIZE];
    u_int32_t ip = ip_addr->ip_addr;

    if(!ip_addr)
        return;
    
    if(!ip_addr->ip_set)
    {
        cli_print(cli, "IP address : N\\A\n");
        return;
    }

    parts[0] = (ip >> 24) & 0xFF;
    parts[1] = (ip >> 16) & 0xFF;
    parts[2] = (ip >> 8) & 0xFF;
    parts[3] = (ip >> 0) & 0xFF;

    cli_print(cli, "IP address : %i.%i.%i.%i\n", parts[0], parts[1], parts[2], parts[3]);
}

void print_subnet(struct cli_def *cli, struct ip_addr *ip_addr)
{
    u_int32_t subnet;
    u_int8_t parts[IP_SIZE];
    if(!cli || !ip_addr)
        return;
    
    if(!ip_addr->ip_set)
    {
        cli_print(cli, "Subnet : N\\A\n");
        return;
    }

    subnet = get_subnet(ip_addr->ip_addr, ip_addr->mask);

    parts[0] = (subnet >> 24) & 0xFF;
    parts[1] = (subnet >> 16) & 0xFF;
    parts[2] = (subnet >> 8) & 0xFF;
    parts[3] = (subnet >> 0) & 0xFF;

    cli_print(cli, "Subnet : %i.%i.%i.%i\n", parts[0], parts[1], parts[2], parts[3]);
}

void print_mac(struct cli_def *cli, struct mac_addr *mac_addr)
{
    if(!mac_addr)
        return;
    cli_print(cli, "MAC address : %X-%X-%X-%X-%X-%X\n", mac_addr->addr[0],
                                mac_addr->addr[1],
                                mac_addr->addr[2],
                                mac_addr->addr[3],
                                mac_addr->addr[4],
                                mac_addr->addr[5]);
}

int set_ip_c(struct cli_def *cli, struct ip_addr *ip_addr, char *value)
{
    u_int32_t ip;
    u_int32_t parts[5];
    u_int8_t mask;

    if((sscanf(value, "%u.%u.%u.%u/%hhu", &parts[0],
                                        &parts[1],
                                        &parts[2],
                                        &parts[3],
                                        &mask)) == EOF)
    {
        cli_error(cli, "Ip address not in format 255.255.255.255/32\n");
        return CLI_ERROR;
    }

    //Ip parts
    parts[0] = (parts[0] & 0xFF) << 24;
    parts[1] = (parts[1] & 0xFF) << 16;
    parts[2] = (parts[2] & 0xFF) << 8;
    parts[3] = (parts[3] & 0xFF) << 0;

    ip = parts[0] | parts[1] | parts[2] | parts[3];

    set_ip(ip_addr, ip, mask);
    cli_print(cli, "Ip address set.\n");
    return CLI_OK;
}




