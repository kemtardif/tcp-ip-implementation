#include "comm_channel.h"

int init_comm_channel(struct graph *graph)
{
    struct doubly_linked_item *item;
    struct pool_item *p_item;
    pthread_t list_pid, snd_pid;
    if(!graph)
        return 0;  
    if((pool = malloc(sizeof(struct fd_pool))) == NULL)
        return 0;
    init_pool(graph, pool);
    sem_init(&mutex, 0, 1);
    cancel_thread = 0;
    if(pthread_create(&list_pid, NULL, listening_thread, (void *)graph)
        || pthread_detach(list_pid))
    {
        printf("listening_thread: %s\n", strerror(errno));
        kill_pool(pool);
        return 0;
    }
    item = pool->fds->head;
    int i =0;
    while(item)
    {
        p_item = (struct pool_item *)item->data;
        if(pthread_create(&snd_pid, NULL, sending_thread, p_item->itf)
            || pthread_detach(snd_pid))
        {
            printf("sending_thread: %s\n", strerror(errno));
            cancel_t();
            kill_pool();
            return 0;
        }
         item = item->next;
    }
    graph->is_up = 1;
    return 1;
}

/*
Still need to be called to free pool in case of 
abrupt termination of threads
*/
void close_comm_channel(struct graph *graph)
{
    if(!graph)
        return;   
    if(graph->is_up)
    {
        cancel_t();
        kill_pool();
    }
    graph->is_up = 0;
}

void *listening_thread(void *arg)
{
    struct graph *graph;
    struct timeval tv;
    if((graph = arg) == NULL)
        return NULL;
    while(1)
    {
        if(IsCancelled())
            break; 
        pool->ready_set = pool->read_set;
        tv.tv_sec = 20;
        tv.tv_usec = 0;
        if((pool->ready = select(pool->max_fd + 1, &pool->ready_set, NULL, NULL, &tv)) == -1)
        {
            printf("select : %s\n", strerror(errno));
            break;
        }
        listen_to_L1s(pool);
    }
    return NULL;
}

/*
There is no concurrency issue on the interface struct itself, 
but on its sending buffer, which is taken care of bu its mutexe/semaphore.
*/
void *sending_thread(void *arg)
{
    struct interface *itf = (struct interface *)arg;
    while(1)
    {
        if(IsCancelled())
            break; 
        send_to_L1s(itf);
    }
    return NULL;
}

void init_pool(struct graph *graph, struct fd_pool *pool)
{
    int i, y = 0;
    struct doubly_linked_item *item, *to_add;
    struct pool_item *pool_item;
    struct graph_node *node;
    if(!graph)
        return;

    pool->fds = dll_init();
    pool->max_fd = 0;
    pool->ready = 0;
    pool->fd_count = 0;
    FD_ZERO(&pool->read_set);
    FD_ZERO(&pool->ready_set);
    item = graph->nodes->head;
    while(item)
    {
        node = (struct graph_node *)item->data;
        item = item->next;
        if(!node)
            continue;
        for(i = 0; i < MAX_INTERFACE; i++)
        {
            if(!node->interfaces[i])
                continue;
            if((pool_item = malloc(sizeof(struct pool_item))) == NULL)
                continue;
            if((to_add = malloc(sizeof(struct doubly_linked_item))) == NULL)
            {
                free(pool_item);
                continue;
            }
            //Initialize pool_item
            pool_item->itf = node->interfaces[i];

            if((pool_item->fd = init_listening_socket()) == -1)
            {
                free(pool_item);
                free(to_add);
                continue;
            }
            pool_item->port = get_port(pool_item->fd);
            node->interfaces[i]->port = pool_item->port;
            FD_SET(pool_item->fd, &pool->read_set);           
            if(pool_item->fd > pool->max_fd)
                pool->max_fd = pool_item->fd;               
            init_io_buffer(&pool_item->fd_buf, 0);
            //Add pool_item to list
            to_add->data = pool_item;
            add_to_list(pool->fds, to_add);
            y++;
        }
    }
    pool->fd_count = y;
}

void free_pool(struct fd_pool *pool)
{
    struct doubly_linked_item *item;
    struct pool_item *pool_item;
    struct interface *itf;
    if(!pool)
        return;
    item = pool->fds->head;
    //Terminate all active connections
    while(!item)
    {
        pool_item = (struct pool_item *)item->data;
        item = item->next;
        if(!pool_item)
            continue;
        close(pool_item->fd);
        pool_item->itf->port = 0;
    }
    //Free list
    dll_free(pool->fds);
    free(pool);
}


unsigned int init_listening_socket()
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
    addr.sin_port = 0;
    if((bind(s, (struct sockaddr *)&addr, addrL)) == -1)
    {
        printf("bind : %s\n", strerror(errno));
        return -1;
    }

    if(listen(s, 3) == -1)
    {
        printf("listen : %s\n", strerror(errno));
        return -1;
    }
    return s;
}

void listen_to_L1s(struct fd_pool *pool)
{
    int i, fd, cd;
    struct doubly_linked_item *item;
    struct pool_item *pool_item;
    struct sockaddr_in addr_in;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    item = pool->fds->head;
    while(item)
    {
        pool_item = (struct pool_item *)item->data;
        item = item->next;
        if(!pool_item)
            continue;   
        fd = pool_item->fd;
        if((fd > 0) && FD_ISSET(fd, &pool->ready_set))
        {
            if((cd = accept(fd, (struct sockaddr *)&addr_in, &addr_len)) == -1)
            {
                printf("accept : %s\n", strerror(errno));
                continue;
            }   
            pool_item->fd_buf.fd = cd;        
            receive_from_L1(pool_item);
            pool_item->fd_buf.fd = 0;
            close(cd);
        }
    }
}


void receive_from_L1(struct pool_item *pool_item)
{
    ssize_t n;
    char usr_buf[MAX_ETH_SIZE];
    printf("Packet received on interface %s at node %s\n", pool_item->itf->name, 
                                                           pool_item->itf->node->name);

    if((n = rio_readnb(&pool_item->fd_buf, usr_buf, sizeof(usr_buf))) <= 0)
    {
        if(n == -1)
            printf("rio_readnb : %s\n", strerror(errno));
        else
            printf("Zero byte received.\n");
        return;
    }
    //Packet sent to link layer
    if(!L1_pass_to_L2(pool_item->itf, usr_buf, n))
        printf("Processing error on node %s\n", pool_item->itf->node->name);  
}        
/*
This the point where the packet is sent from the link layer to the physical layer.
*/
int L2_pass_to_L1(char *packet, size_t pckt_size, struct interface *itf)
{
    int fd;
    struct interface *attached;
    unsigned int port;
    char port_s[6];
    if(!packet || !pckt_size || !itf)
        return 0;
    if((attached = get_attached_interface(itf)) == NULL)
    {
        printf("Sending interface not linked.\n");
        return 0;
    }
    if((port = attached->port) == 0)
    {
        printf("Attached node not connected.\n");
        return 0;
    }    
    if(sprintf(port_s, "%u", port) <= 0)
    {
        printf("Could not parse port.\n");
        return 0;
    }  
    if((fd = get_send_socket(port_s)) == -1)
    {
        printf("Could not create sending socket.\n");
        return 0;
    }
    if(rio_writen(fd, (void *)packet, pckt_size) == -1)
    {
         printf("rio_writen : %s\n", strerror(errno));
         close(fd); 
         return 0;
    }
    close(fd);    
    return 1;
}

void receive_from_L5(struct graph_node *node, char *data, size_t data_size)
{
    L5_pass_to_L4(node, data, data_size);
}

int L5_pass_to_L4(struct graph_node *node, char * packet, size_t pckt_size)
{
    //Transport layer stuff
    //FOR NOW WE JUST PASS MESSAGE AS IS TO LOWER LAYER
    L4_pass_to_L3(node, packet, pckt_size);
    return 1;
}
//Pass data from transport layer to network layer
int L4_pass_to_L3(struct graph_node *node, char * packet, size_t pckt_size)
{
    int i;

    for(i = 0; i < MAX_INTERFACE; i++)
    {
        if(node->interfaces[i])
            L3_pass_to_L2(node->interfaces[i], packet, pckt_size);
    }
        return 1;
}

int L3_pass_to_L2(struct interface *snd_itf, char *packet, size_t pckt_size)
{
    char ethernet_pckt[MAX_ETH_SIZE];
    size_t eth_size;
    //Do L2 stuff with L3 data, including adding ethernet headers and ARP resolution
    //FOR NOW, WE JUST ATTACH ETHERNET HEADERS FOR BROADCASTING AND PASS TO INTERFACE QUEUE
    u_int8_t dstMac[MAC_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    if(prepare_ethernet_packet(ethernet_pckt, packet, pckt_size, snd_itf->mac_addr, dstMac, IP_TYPE, &eth_size))
        add_packet_to_send_queue(snd_itf, ethernet_pckt, eth_size);
    return 1;
}

void send_to_L1s(struct interface *itf)
{
    struct interface *snd_itf;
    struct send_packet *packet;
    int send = 4;
    if((snd_itf = itf) == NULL)
        return;
    while(send > 0 && ((packet = (struct send_packet *)pop(snd_itf->send_queue)) != NULL))
    {
        if(!L2_pass_to_L1(packet->packet, packet->pckt_size, snd_itf))
        {
            packet->retransmit--;
            if(!packet->retransmit)  //Drop packet after enough transmission try
            {
                printf("Dropped sending packet at interface %s\n", snd_itf->name);
                free(packet->packet);
                free(packet);
            } else if(!push(snd_itf->send_queue, packet)) //Packet sent back in queue
                printf("Could not queue back packet at interface %s\n", snd_itf->name); 
        } 
        else //Transmission succesful
        {
            free(packet->packet);
            free(packet);
        }
        send--;
    }
}

int L1_pass_to_L2(struct interface *rcv_itf, char * packet, size_t pckt_size)
{

    struct eth_frame eth_frame;
    if(!rcv_itf || !packet || !pckt_size)
        return 0;  
    //Extract ethernet headers
    if(remove_ethernet_headers(&eth_frame, packet, pckt_size) == NULL)
        return 0;
    //Set interface to pass message to
    switch(rcv_itf->node->type)
    {
        case SWITCH:
            process_switch(rcv_itf, &eth_frame, packet, pckt_size);
            break;
        case HOST :
            printf("Packet received at host %s\n", rcv_itf->node->name);
            break;
        default:
            printf("NO TYPE FOUND\n");
            break;
    }
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
int IsCancelled()
{
    sem_wait(&mutex);
        if(cancel_thread)
        {
            cancel_thread--;
            sem_post(&mutex); 
            return 1;
        }  
    sem_post(&mutex);
    return 0;
}
void cancel_t()
{
    sem_wait(&mutex);
    cancel_thread++;
    sem_post(&mutex);
}
void kill_pool()
{
    if(pool)
        free_pool(pool);
    pool = NULL;
}
unsigned int get_port(int fd)
{
    struct sockaddr_in addr_out;
    socklen_t len_out;
    if(getsockname(fd, (struct sockaddr *)&addr_out, &len_out) == -1)
    {
        printf("getsockname : %s\n", strerror(errno));
        return -1;
    }
    return ntohs(addr_out.sin_port);
}



