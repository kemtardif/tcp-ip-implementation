#include "comm_channel.h"

int init_comm_channel(struct graph *graph)
{
    pthread_t comm_pid;

    if(!graph)
        return 0;  
    if(pthread_create(&comm_pid, NULL, comm_thread, (void *)graph))
        return 0;
    if(pthread_detach(comm_pid))
        return 0;

    graph->is_up = comm_pid;
    return 1;
}

void close_comm_channel(struct graph *graph)
{
    if(!graph)
        return;   

    close_sockets(graph);

    if(graph->is_up)
        pthread_cancel(graph->is_up);
    graph->is_up = 0;
    printf("Comm channel is closed\n.");
}

void *comm_thread(void *arg)
{
    struct graph *graph = (struct graph *)arg;
    struct fd_pool pool;
    if(init_sockets(graph) == -1)
    {
        printf("Could not activate sockets.\n");
        return NULL;
    }
    pool_init(graph, &pool);

    while(1)
    {
        pool.ready_set = pool.read_set;

        if((pool.ready = select(pool.max_fd + 1, &pool.ready_set, NULL, NULL, NULL)) == -1)
        {
            printf("select : %s", strerror(errno));
            close_sockets(graph);
            return NULL;
        }
        accept_connections(&pool);
    }
    return NULL;
}

int init_sockets(struct graph *graph)
{
    struct doubly_linked_item *item;
    struct graph_node *node;

    if(!graph)
        return -1;
    item = graph->nodes->head;

    while(item)
    {
        node = (struct graph_node *)item->data;
        node->socket_port = get_new_port(graph);
        node->socket_fd = init_comm_socket(node);
        printf("NODE : %s -- PORT : %u -- FD : %i\n", node->name, node->socket_port, node->socket_fd);

        item = item->next;
    }
    return 1;
}

int close_sockets(struct graph * graph)
{
    struct doubly_linked_item *item;
    struct graph_node *node;
    int fd;

    if(!graph)
        return -1;

    item = graph->nodes->head;
    while(item)
    {
        node = (struct graph_node *)item->data;
        fd = node->socket_fd;

        if(fd == -1)
            continue;

        close(fd);
        node->socket_fd = -1;
        node->socket_port = 0;
        
        item = item->next;
    }
}

void pool_init(struct graph *graph, struct fd_pool *pool)
{
    int i = 0;
    struct doubly_linked_item *item;
    struct graph_node *node;

    pool->max_fd = 0;
    pool->ready = 0;
    pool->fd_count = 0;
    memset(pool->fd_pool, -1, sizeof(pool->fd_pool));
    FD_ZERO(&pool->read_set);
    FD_ZERO(&pool->ready_set);

    if(!graph)
        return;
    item = graph->nodes->head;

    while(item)
    {
        node = (struct graph_node *)item->data;

        if(node->socket_fd == -1)
            continue;

        pool->fd_pool[i] = node->socket_fd;
        pool->nodes[i] = node;
        FD_SET(pool->fd_pool[i], &pool->read_set);

        if(pool->fd_pool[i] > pool->max_fd)
            pool->max_fd = pool->fd_pool[i];

        init_io_buffer(&pool->fd_buf[i], 0);

        item = item->next;
        i++;
    }
    pool->fd_count = i;
}

unsigned int get_new_port(struct graph *graph)
{
    int i, port = INIT_PORT;
    struct doubly_linked_item *item;
    struct graph_node *node;

    item = graph->nodes->head;

    if(!item)
        return 0;

    while(item)
    {
        node = (struct graph_node *)item->data;
        if(node->socket_port > port)
            port = node->socket_port;
        item = item->next;
    }
    return (port + 1);
}
unsigned int init_comm_socket(struct graph_node *node)
{
    struct sockaddr_in addr;
    socklen_t addrL = sizeof(addr);
    int x, s, opt = 1;

    //Create socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(int));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(node->socket_port);
    if((bind(s, (struct sockaddr *)&addr, addrL)) == -1)
    {
        printf("bind : %s", strerror(errno));
        return -1;
    }

    if(listen(s, 1) == -1)
    {
        printf("listen : %s", strerror(errno));
        return -1;
    }
    return s;
}

void accept_connections(struct fd_pool *pool)
{
    int i, fd, cd, c = 0;
    struct sockaddr_in addr_in;
    socklen_t addr_len;

    for(i = 0; i < MAX_NODE; i++)
    {
        fd = pool->fd_pool[i];
        if(fd == -1)
            continue;

        if(FD_ISSET(fd, &pool->ready_set))
        {

            if((cd = accept(fd, (struct sockaddr *)&addr_in, &addr_len)) == -1)
            {
                printf("accept : %s\n", strerror(errno));
                continue;
            }
            process_connection(pool->nodes[i], &pool->fd_buf[i], cd);
            close(cd);

            if(++c == pool->fd_count)
                break;
        }
    }
}

/*
Upon receiption, we strip the receiving interface name,
so that the node process the actual data "coming from
that interface"
*/
void process_connection(struct graph_node *node, struct io_buffer *io_buf, int fd)
{
    ssize_t n, actual;
    char usr_buf[MAX_PACKET_SIZE];
    struct interface *itf;

    io_buf->fd = fd;
    if((n = rio_readnb(io_buf, usr_buf, sizeof(usr_buf))) == -1)
    {
        printf("rio_readnb : %s\n", strerror(errno));
        io_buf->fd = 0;
        return;
    }
    else if (!n)
        return;
    if((itf = find_interface_by_name(node, usr_buf)) == NULL)
    {
        printf("Interface %s not on node %s\n", usr_buf, node->name);
        return;
    }    
    if(!rcv_pack_at_node_via_interface(node, itf, (char*)usr_buf + INT_NAME_SIZE, n - INT_NAME_SIZE))
    {
        printf("Processing error for packet from interface %s of node %s", itf->name, node->name);
    }
}

/*
From the sending interface, we prepend the attached interface name to the data
for the receiver to know on which interface it received the message.
This the point where the packet is sent to the link layer.
*/
int send_pckt_via_interface(char *packet, size_t pckt_size, struct interface *itf)
{
    int fd;
    struct interface *attached;
    unsigned int port;
    char port_s[6];
    char *modified_pckt;
    if(!packet || !pckt_size || !itf)
        return -1;
    if((attached = get_attached_interface(itf)) == NULL)
    {
        printf("Sending interface not linked.\n");
        return -1;
    }
    if((port = attached->node->socket_port) == 0)
    {
        printf("Attached node not connected.\n");
        return -1;
    }    
    if(sprintf(port_s, "%u", port) <= 0)
    {
        printf("Could not parse port.\n");
        return -1;
    }  
    if((fd = get_send_socket(port_s)) == -1)
    {
        printf("Could not create sending socket.\n");
        return -1;
    }
    modified_pckt = malloc(INT_NAME_SIZE + pckt_size);
    if(!create_modified_packet(packet, attached->name, modified_pckt, pckt_size))
    {
        printf("Could not create modified packet.\n");
        free(modified_pckt);
        return -1;
    }
    if(rio_writen(fd, (void *)modified_pckt, INT_NAME_SIZE + pckt_size) == -1)
        printf("rio_writen : %s", strerror(errno));

    free(modified_pckt);
    close(fd);    
    return 1;
}

/*
This is the entry point from the link layer into a node.
*/
int rcv_pack_at_node_via_interface(struct graph_node *node, struct interface *itf, char *packet, size_t pck_size)
{
    if(!node || !itf || !packet || !pck_size)
        return 0;

    printf("%ld bytes received on interface %s  of node %s\n", pck_size, itf->name, node->name);
    printf("Data : %s\n",packet);

    return 1;
}

int get_send_socket(char *port_s)
{
    int fd, err;
    struct addrinfo hints, *list, *p;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV; 
    if((err = getaddrinfo(LOCALHOST, port_s, &hints, &list)) != 0)
    {
        printf("getaddrinfo :%s\n", gai_strerror(err));
        return -1;
    }

    for (p = list; p; p = p->ai_next) 
    {

        if((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; 
        if (connect(fd, p->ai_addr, p->ai_addrlen) != -1)
            break;       
        close(fd); 
    }
    freeaddrinfo(list);

    if(!p)
        return -1;
    else
        return fd;
}

int create_modified_packet(char *init_pck, char *itf_name, char *fnl_pckt, size_t pckt_size)
{
    size_t i;
    if(!init_pck || !itf_name || !fnl_pckt || !pckt_size)
        return 0;
    memcpy(fnl_pckt, itf_name, INT_NAME_SIZE);
    memcpy(fnl_pckt + INT_NAME_SIZE, init_pck, pckt_size);
    return 1;
}

int broadcast_from(struct graph_node *node, char *packet, size_t pck_size, struct interface *except)
{
    int i, count = 0;
    struct interface *current;

    if(!node || !packet || !pck_size)
        return -1;
    for(i = 0; i < MAX_INTERFACE; i++)
    {
        current = node->interfaces[i];
        if(!current || current == except)
            continue;
        if(send_pckt_via_interface(packet, pck_size, current) != -1)
            count++;
    }
    return count;
}


