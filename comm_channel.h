#ifndef _COMM_CHANNEL_H_
#define _COMM_CHANNEL_H_
#include <pthread.h>
#include <sys/select.h>
#include <netdb.h>
#include "structures/dll.h"
#include "structures/graph.h"
#include "io_ops/io.h"

#define MAX_PACKET_SIZE 1500
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
int send_pckt_via_interface(char *packet, size_t pckt_size, struct interface *itf);
int rcv_pack_at_node_via_interface(struct graph_node *node, struct interface *itf, char *packet, size_t pck_size);
int get_send_socket(char *port_s);
int create_modified_packet(char *init_pck, char *itf_name, char *fnl_pckt, size_t pckt_size);
int broadcast_from(struct graph_node *node, char *packet, size_t pck_size, struct interface *except);
#endif