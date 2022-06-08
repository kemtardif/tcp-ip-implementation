#ifndef _IO_H_
#define _IO_H_
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define MAX_BUF_SIZE 8192

/*
    This is a standard IO package suited for networking. It handle reads
    for unknown number of bits and prevent early termination by re-reading 
    if syscall interrupts happen. 
    Taken from "Computer Systems - A Programmer’s Perspective" by Bryant & O’Hallaron.
*/

struct io_buffer {
    int fd;
    ssize_t counter;
    char *next_unread;
    char buf[MAX_BUF_SIZE];
};

void init_io_buffer(struct io_buffer *io_buf, int fd);
ssize_t rio_read(struct io_buffer *io_buf, char *data_buffer, size_t n);
ssize_t rio_readnb(struct io_buffer *io_buf, void *data_buffer, size_t n);
ssize_t rio_readline(struct io_buffer*io_bud, void *data_buf, size_t max);
ssize_t rio_writen(int fd, void *usr_buf, size_t n);
#endif