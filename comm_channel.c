#include "comm_channel.h"

int init_comm_channel(struct graph *graph)
{
    struct doubly_linked_item *item;
    struct pool_item *p_item;
    pthread_t snd_pid;

    if(!graph)
        return 0; 

    sem_init(&mutex, 0, 1);
    cancel_thread = 0;

    if(!init_pool(graph))
        return 0;
    if(!init_listening_thread(graph))
        return 0;
    if(!init_sending_threads(graph))
        return 0;
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
        pthread_join(pool->list_pid, NULL);
        /*
            Wait for listening thread termination
            At this point, pool sockets are closed
        */
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
    while(IsRunning())
    {
        pool->ready_set = pool->read_set;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        if((pool->ready = select(pool->max_fd + 1, &pool->ready_set, NULL, NULL, &tv)) == -1)
        {
            printf("select : %s\n", strerror(errno));
            break;
        }
        listen_to_L1s(pool);
    }
    terminate_sending_threads();
    close_listening_sockets();
    printf("Listening thread closed\n");
    return NULL;
}

/*
There is no concurrency issue on the interface struct itself, 
but on its sending buffer, which is taken care of bu its mutexe/semaphore.
*/
void *sending_thread(void *arg)
{
    int keep_going = 1;
    struct interface *itf = (struct interface *)arg;
    while(keep_going)
         keep_going = send_to_L1s(itf);

    free_sending_thread(itf);
    printf("Sending thread cleaned and closed.\n");
    return NULL;
}

int init_pool(struct graph *graph)
{
    int i, y = 0;
    struct doubly_linked_item *item;
    struct graph_node *node;
    if(!graph)
        return 0;

    if((pool = malloc(sizeof(struct fd_pool))) == NULL)
        return 0;
    set_init_pool_state();

    item = graph->nodes->head;
    while(item)
    {
        node = (struct graph_node *)item->data;
        item = item->next;
        if(!node)
            continue;
        for(i = 0; i < MAX_INTERFACE; i++)
        {
            if(!set_pool_item_at_interface(node->interfaces[i]))
                continue;
            y++;
        }
    }
    pool->fd_count = y;
    return 1;
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
    while(item)
    {
        pool_item = (struct pool_item *)item->data;
        item = item->next;
        if(!pool_item)
            continue;
        if(pool_item->fd != -1)
            close(pool_item->fd);
        free(pool_item);
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
    socklen_t addr_len;

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
            addr_len = sizeof(struct sockaddr_in);
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

void receive_from_L5(struct graph_node *node, char *data, size_t data_size, u_int32_t ip_dest)
{
    //FOR NOW WE JUST PASS MESSAGE AS IS TO LOWER LAYER
    L5_pass_to_L4(node, data, data_size, ip_dest);
}

int L5_pass_to_L4(struct graph_node *node, char * packet, size_t pckt_size, u_int32_t ip_dest)
{
    //Transport layer stuff
    //FOR NOW WE JUST PASS MESSAGE AS IS TO LOWER LAYER
    L4_pass_to_L3(node, packet, pckt_size, ip_dest);
    return 1;
}
//Pass data from transport layer to network layer
int L4_pass_to_L3(struct graph_node *node, char *packet, size_t pckt_size, u_int32_t ip_dest)
{
    int i;
    size_t ip_size;
    struct interface *snd_itf;
    char ip_packet[MAX_IP_SIZE];
    //We ommit routing protocols for now, which determine next hop router and
    //sending interface. For now, we use only host, which has a single interface.
    for(i = 0; i < MAX_INTERFACE; i++)
    {
        if((snd_itf = node->interfaces[i]) != NULL)
            break;
    }
    if(!snd_itf)
        return 0;      
    if(!prepare_ip_packet(ip_packet, packet, pckt_size, 
                            snd_itf->ip.ip_addr, 
                            ip_dest, &ip_size))
        return 0;
     if(!L3_pass_to_L2(snd_itf, ip_packet, ip_size, ip_dest))
        return 0;
    return 1;
}

int L3_pass_to_L2(struct interface *snd_itf, char *packet, size_t pckt_size, u_int32_t ip_dest)
{
    int resolved;
    char ethernet_pckt[MAX_ETH_SIZE], *arp_pckt;
    size_t eth_size, arp_size;
    u_int8_t dstMac[MAC_SIZE];

    if((resolved = resolve_target_ip(snd_itf, ip_dest, dstMac)) == RSLVD_ERR)
        return 0;
    else if(resolved == RSLVD)
    {
        if(!prepare_ethernet_packet(ethernet_pckt, packet, pckt_size, snd_itf->mac_addr, dstMac, IPV4_TYPE, &eth_size))
            return 0;
        add_packet_to_send_queue(snd_itf, ethernet_pckt, eth_size);
    } else
    {
        //Unresolved. Buffer packet, send arp_request. dstMac is broadcast
        if(!buffer_packet_arp(snd_itf, packet, pckt_size, ip_dest))
            return 0;
        if((arp_pckt = create_arp_packet(snd_itf, ip_dest, NULL, &arp_size, REQ_OP)) == NULL)
            return 0;
        if(!prepare_ethernet_packet(ethernet_pckt, arp_pckt, arp_size, snd_itf->mac_addr, dstMac, ARP_TYPE, &eth_size))
        {
            free(arp_pckt);
            return 0;
        }
        broadcast_to_send_queues(snd_itf->node, NULL, ethernet_pckt, eth_size);
        free(arp_pckt);
    }
    return 1;
}

int send_to_L1s(struct interface *itf)
{
    struct interface *snd_itf;
    struct send_packet *packet;
    int send = 4;
    if((snd_itf = itf) == NULL)
        return 0;
    while(send > 0 && ((packet = (struct send_packet *)pop(snd_itf->send_queue)) != NULL))
    {
        if(is_kill_packet(packet))
        {
            free(packet->packet);
            free(packet);     
            printf("Freed kill packet\n");       
            return 0;
        }
        if(!L2_pass_to_L1(packet->packet, packet->pckt_size, snd_itf))
        {
            packet->retransmit--;
            if(!packet->retransmit)  //Drop packet after enough transmission try
            {
                printf("Dropped sending packet at interface %s\n", snd_itf->name);
                free(packet->packet);
                free(packet);
            } 
            else 
                push(snd_itf->send_queue, packet); //Packet sent back in queue
        } 
        else //Transmission succesful
        {
            free(packet->packet);
            free(packet);
        }
        send--;
    }
    return 1;
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
            process_L2_switch(rcv_itf, &eth_frame, packet, pckt_size);
            break;
        case HOST :
            if(can_process_eth(rcv_itf, &eth_frame))
                L2_pass_to_L3_by_type(rcv_itf, &eth_frame, packet, pckt_size);
            else    
                printf("Packet not meant for interface %s. Drop packet.\n", rcv_itf->node->name);
            break;
        default:
            printf("NO valid interface type. Drop packet.\n");
            break;
    }
    return 1;
}

void L2_pass_to_L3_by_type(struct interface *rcv_itf, struct eth_frame *eth_frame, char * packet, size_t pckt_size)
{
    if(!rcv_itf || !eth_frame || !packet || !pckt_size)
        return;

    switch(eth_frame->type)
    {
        case IPV4_TYPE:
            L2_pass_to_L3_ip(rcv_itf, eth_frame->data, eth_frame->data_size);
            break;
        case ARP_TYPE:
            L2_pass_to_L3_arp(rcv_itf, eth_frame->data, eth_frame->data_size);
            break;
        default:
            printf("No know ether-type. Drop packet.\n");
        break;
    }
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
        freeaddrinfo(list);
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
int IsRunning()
{
    sem_wait(&mutex);
        if(cancel_thread)
        {
            cancel_thread--;
            sem_post(&mutex); 
            return 0;
        }  
    sem_post(&mutex);
    return 1;
}
void cancel_t()
{   
    sem_wait(&mutex);
    cancel_thread++;
    sem_post(&mutex);
}

void terminate_sending_threads()
{
    struct send_packet *packet;
    struct doubly_linked_item *dll_item;
    struct pool_item *item;
    struct interface *itf;

    dll_item = pool->fds->head;
    while(dll_item)   
    {
        item = (struct pool_item *)dll_item->data;
        itf = (struct interface *)item->itf;
        packet = create_kill_packet();
        push(itf->send_queue, packet);
        dll_item = dll_item->next;
    }
}
void *create_kill_packet()
{
    struct send_packet *packet;
    char *stop = "STOP";
    if((packet = (struct send_packet *)malloc(sizeof(struct send_packet))) == NULL)
        return NULL;
    if((packet->packet = malloc(strlen(stop) + 1)) == NULL)
    {
        free(packet);
        return NULL;
    }
    strcpy(packet->packet, stop);
    packet->pckt_size = strlen(stop) + 1;
    packet->retransmit = 4;
    printf("Created kill packet\n");
    return packet;
}
void kill_pool()
{
    if(pool)
        free_pool(pool);
    pool = NULL;
}

void set_init_pool_state()
{
    pool->fds = dll_init();
    pool->max_fd = 0;
    pool->ready = 0;
    pool->fd_count = 0;
    FD_ZERO(&pool->read_set);
    FD_ZERO(&pool->ready_set);
}

int init_listening_thread(struct graph *graph)
{
    if(pthread_create(&pool->list_pid, NULL, listening_thread, (void *)graph))
    {
        printf("listening_thread: %s\n", strerror(errno));
        kill_pool(pool);
        return 0;
    }
    return 1;
}

void *init_pool_item()
{
    struct pool_item *p_item;
    if((p_item = malloc(sizeof(struct pool_item))) == NULL)
        return NULL;
    if((p_item->fd = init_listening_socket()) == -1)
    {
        free(p_item);
        return NULL;
    }
    p_item->port = get_port(p_item->fd);
    init_io_buffer(&p_item->fd_buf, 0);

    FD_SET(p_item->fd, &pool->read_set);           
    if(p_item->fd > pool->max_fd)
        pool->max_fd = p_item->fd; 

    return p_item;
}

int set_pool_item_at_interface(struct interface *itf)
{
    struct doubly_linked_item *item, *to_add;
    struct pool_item *pool_item;
    if(!itf)
        return 0;;
    if((to_add = malloc(sizeof(struct doubly_linked_item))) == NULL)
        return 0;
    if((pool_item = init_pool_item()) == NULL)
    {
        free(to_add);
        return 0;;
    }
    pool_item->itf = itf;
    itf->port = pool_item->port;             
    //Add pool_item to list
    to_add->data = pool_item;
    add_to_list(pool->fds, to_add);
    return 1;
}

int init_sending_threads(struct graph *graph)
{
    struct doubly_linked_item *item;
    struct pool_item *p_item;
    pthread_t snd_pid;
    if(!graph)
        return 0;  
    item = pool->fds->head;
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
    return 1;
}

unsigned int get_port(int fd)
{
    struct sockaddr_in addr_out;
    socklen_t len_out = sizeof(struct sockaddr_in);
    if(getsockname(fd, (struct sockaddr *)&addr_out, &len_out) == -1)
    {
        printf("getsockname : %s\n", strerror(errno));
        return -1;
    }
    return ntohs(addr_out.sin_port);
}

void empty_queue(struct queue *queue)
{
    struct send_packet *packet;
    if(!queue)
        return;
    while(queue->front < queue->rear)
    {
        packet = (struct send_packet *)queue->array[(++queue->front) % queue->capacity];
        free(packet->packet);
        free(packet);
    }
    return;
}
void empty_arp_table(struct hash_table *ht)
{
    int i;
    struct hash_entry entry;
    if(!ht)
        return;
    for(i = 0; i < ht->capacity; i++)
    {
        entry = ht->entries[i];
        if(entry.key)
            free(entry.key);
        if(entry.to_free)
            free(entry.value);
        ht->entries[i].key = NULL;
        ht->entries[i].value = NULL;
    }
    return;
}
void empty_arp_buffer(struct interface *itf)
{
    int i;
    if(!itf)
        return;

    for(i = 0; i < ARP_BUFFER_LENGTH; i++)
    {
        if(itf->arp_buffer[i].packet)
            free(itf->arp_buffer[i].packet);
    }
    return;
}
int is_kill_packet(struct send_packet *packet)
{
    return (!strncmp(packet->packet, "STOP", packet->pckt_size));
}

void free_sending_thread(struct interface *itf)
{
    empty_queue(itf->send_queue);
    if(itf->node->type != SWITCH)
    {
        empty_arp_table(itf->arp_table);
        empty_arp_buffer(itf);
    }
}
void close_listening_sockets()
{
    struct doubly_linked_item *dll_item;
    struct pool_item *item;

    dll_item = pool->fds->head;
    while(dll_item)   
    {
        item = (struct pool_item *)dll_item->data;       
        close(item->fd);
        item->fd = -1;
        dll_item = dll_item->next;
    }
}



