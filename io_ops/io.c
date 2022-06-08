#include "io.h"

void init_io_buffer(struct io_buffer *io_buf, int fd)
{
    io_buf->fd = fd;
    io_buf->counter = 0;
    io_buf->next_unread = io_buf->buf;
}

/*
Read n bits into dfata_buffer.
IO buffer read at most MAX_BUF_SIZE bits from file descriptor.
If more than n bits are read, then n bits are copied to user buffer
and IO buffer point to next unread bit, with counter the remaining bits.
If IO buffer read less than n bits, those are copied to user buffer,
IO buffer oint to next unread bit (free slot)
and counter is set to zero.
*/
ssize_t rio_read(struct io_buffer *io_buf, char *data_buffer, size_t n)
{

    ssize_t cnt;
    while(io_buf->counter <= 0)
    {
        if((io_buf->counter = read(io_buf->fd, io_buf->buf, sizeof(io_buf->buf))) < 0)
        {
            if(errno != EINTR)
                return -1; //Return for error not interrupt
            
        }
        else if (io_buf->counter == 0)
                return 0; //EOF
        else 
            io_buf->next_unread = io_buf->buf;
    }
    cnt = n;
    if(io_buf->counter < n)
        cnt = io_buf->counter;
    memcpy(data_buffer, io_buf->buf, cnt);
    io_buf->next_unread += cnt;
    io_buf->counter -= cnt;
    return cnt;
}

/*
Keep reading until error or EOF, return number of bytes read.
*/
ssize_t rio_readnb(struct io_buffer *io_buf, void *data_buffer, size_t n)
{
    size_t left = n;
    ssize_t read;
    char * buf = data_buffer;
    while(left > 0)
    {
        if((read = rio_read(io_buf, buf, left)) < 0)
            return -1;
        else if(read == 0)
            break;        
    
        left -= read;
        buf += read;
    }

    return (n - left);
}

ssize_t rio_readline(struct io_buffer*io_buf, void* data_buf, size_t max)
{
    int n, rc;
    char c, *buf = data_buf;

    for(n = 1; n < max; n++)
    {
        if((rc == rio_read(io_buf, &c, 1)) == 1)
        {
            *buf++ = c;
            if(c == '\n')
            {
                n++;
                break;
            }
        } 
        else if(rc == 0)
        {
            if(n == 1)
                return 0; //No data
            else
                break; //Some reading
        }
        else
            return -1;
    }
    *buf = '\n';
    return (n - 1);

}

ssize_t rio_writen(int fd, void *usr_buf, size_t n)
{
    size_t left = n;
    ssize_t written;
    char *buf = (char *)usr_buf;

    if(!usr_buf || !n)
        return -1;

    while(left > 0)
    {
        if((written = write(fd, buf, left)) <= 0)
        {
            if(errno == EINTR)
                written = 0; //Interrup, re-write
            else 
                return -1;
            
            
        }
        left -= written;
        buf += written;
    }
    return n;
}