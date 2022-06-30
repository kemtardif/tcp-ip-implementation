#ifndef _COMM_CHANNEL_H_
#define _COMM_CHANNEL_H_
#include <pthread.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netdb.h>
#include <semaphore.h>
#include "structures/dll.h"
#include "structures/graph.h"
#include "structures/queue.h"
#include "ethernet.h"
#include "ip.h"
#include "packet.h"
#include "io_ops/io.h"
#include "switch.h"
#include "arp.h"

//Used to cancel listening thread by main
volatile long cancel_thread;
sem_t mutex;

#define LOCALHOST "localhost"
struct fd_pool 
{
    fd_set ready_set;
    fd_set read_set;
    pthread_t list_pid;
    int max_fd;
    int fd_count;
    int ready;
    struct doubly_linked_list *fds;
};
struct pool_item
{
    int fd;
    unsigned int port;
    struct io_buffer fd_buf;
    struct interface *itf;
};

struct fd_pool *pool;

//Initialize comm_channel, including all socket and port on interfaces
int init_comm_channel(struct graph *graph);
//Close all connection and reset interfaces
void close_comm_channel(struct graph *graph);
/*Initialize communication channel for interfaces. 
This represent the L1 channel.
*/
unsigned int init_listening_socket();
/*This is the listening thread for transmission arriving to
interfaces via link. Received packets are buffered and
sent to the link layer via a specific interface.
*/
void *listening_thread(void *arg);
/*
This is a thread dedicated to sending buffered messages at interface.
The packets in the sending buffers are ready to be sent.
This is mainly used to decouple IO operations and OSI layers.

*/
void *sending_thread(void *arg);
/*
Initialize pool of file dcescriptors/sockets
*/
int init_pool(struct graph *graph);
//Free pool, close sockets and set interface ports to 0
void free_pool(struct fd_pool *pool);
/*
Accept incoming transmission when there is data to be read on
any file descripttor
*/
void listen_to_L1s(struct fd_pool *pool);
/*
Transmit up to 4 packets on each interfaces.
Packet are pushed back in queue on sending error, or dropped
after enough tries.
*/
int send_to_L1s(struct interface *itf);

/*
The next few functions represent de various OSI layers
*/
/*
Receive transmission arriving from L1
*/
void receive_from_L1(struct pool_item *pool_item);
//Pass arriving packet on L1 to L2 layer
int L1_pass_to_L2(struct interface *rcv_itf, char * packet, size_t pckt_size);
//Pass data to transport layer by type
void L2_pass_to_L3_by_type(struct interface *rcv_itf, struct eth_frame *eth_frame, char * packet, size_t pckt_size);
//Pass data from network layer to transport layer
int L3_pass_to_L4(struct interface *rcv_itf, char * packet, size_t pckt_size);
//Pass data from transport layer to application layer
int send_on_L5(struct interface *rcv_itf, char * packet, size_t pckt_size);

//Receive data from application layer at node
void receive_from_L5(struct graph_node *node, char *data, size_t data_size, u_int32_t ip_dest);
//Pass data from application layer to transport layer
int L5_pass_to_L4(struct graph_node *node, char * packet, size_t pckt_size, u_int32_t ip_dest);
//Pass data from transport layer to network layer
int L4_pass_to_L3(struct graph_node *node, char * packet, size_t pckt_size, u_int32_t ip_dest);
//Pass data from network layer to link
int L3_pass_to_L2(struct interface *snd_itf, char * packet, size_t pckt_size, u_int32_t ip_dest);

int get_send_socket(char *port_s);
int IsRunning();
void cancel_t();
void kill_pool();
void set_init_pool_state();
int init_listening_thread(struct graph *graph);
void *init_pool_item();
int set_pool_item_at_interface(struct interface *itf);
int init_sending_threads(struct graph *graph);
void terminate_sending_threads();
void *create_kill_packet();
unsigned int get_port(int fd);
void empty_queue(struct queue *queue);
void empty_arp_table(struct hash_table *ht);
void empty_arp_buffer(struct interface *itf);
int is_kill_packet(struct send_packet *packet);
void free_sending_thread(struct interface *itf);
void close_listening_sockets();
#endif