#ifndef _COMM_CHANNEL_H_
#define _COMM_CHANNEL_H_
#include <pthread.h>
#include <sys/select.h>
#include <netdb.h>
#include <semaphore.h>
#include "structures/dll.h"
#include "structures/graph.h"
#include "ethernet.h"
#include "packet.h"
#include "io_ops/io.h"


//Used to cancel thread by main
volatile long cancel_thread;
sem_t mutex;

#define LOCALHOST "localhost"
struct fd_pool 
{
    fd_set ready_set;
    fd_set read_set;
    int max_fd;
    int fd_count;
    int ready;
    int fd_pool[MAX_NODE];
    struct io_buffer fd_buf[MAX_NODE];
    struct graph_node *nodes[MAX_NODE];
};
//Initialize comm_channel, including all sockets and ports
int init_comm_channel(struct graph *graph);
//Close all connection and reset nodes
void close_comm_channel(struct graph *graph);
//Return a unique port number starting at 4000, one unit higher than last
//used port.
unsigned int get_new_port(struct graph *graph);
//Initialize communication channel for a node. This represent the L1-layer
//sending information below.
unsigned int init_comm_socket(struct graph_node *node);
void *comm_thread(void *arg);
int init_sockets(struct graph *graph);
int close_sockets(struct graph * graph);
void pool_init(struct graph *graph, struct fd_pool *pool);
void accept_connections(struct fd_pool *pool);
void process_connection(struct graph_node *node, struct io_buffer *io, int fd);
//Writing to buffer
int write_via_interface(char *packet, size_t pckt_size, struct interface *itf);
//prepare and send packet to the interface connected to input. Return zero on error.
int send_packet(struct interface *snd_itf, char *packet, size_t pck_size, u_int16_t type, int is_broadcast);
int get_send_socket(char *port_s);
int broadcast_from(struct graph_node *node, char *packet, size_t pck_size, u_int16_t type, struct interface *except);
#endif