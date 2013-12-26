/* this file contains functions that make dealing with the network a bit
   easier */
#include "state.h"

/* this function just runs send(2) in a loop until the entire buffer is sent or
   an error happens */

ssize_t send_all(int socket, const void *buffer, size_t length, int flag)
{
    size_t bytes_sent = 0;
    size_t bytes_unsent = length;

    int sent;

    while (bytes_sent < length)
    {
        sent = send(socket, buffer + bytes_sent, bytes_unsent, flag);
        if (sent == -1) 
            return -1;
        bytes_sent += sent;
        bytes_unsent -= sent;
    }
    
    return bytes_sent;
}

/* just runs recv(2) in a loop until an error happens or our peer shuts down 
   the connection */

ssize_t recv_all(int socket, void *buffer, size_t length, int flag)
{
    size_t bytes_received = 0;
    size_t bytes_unreceived = length;

    int received;

    while (bytes_received < length)
    {
        received = recv(socket, buffer + bytes_received, bytes_unreceived, flag);
        if (received == -1) 
            return -1;
        if (received == 0) 
            return 0;
        bytes_received += received;
        bytes_unreceived -= received;
    }
    
    return bytes_received;
}




ssize_t write_all(int fd, const void *buffer, size_t count)
{
    size_t bytes_sent = 0;
    size_t bytes_unsent = count;

    int sent;

    while (bytes_sent < count)
    {
        sent = write(fd, buffer + bytes_sent, bytes_unsent);
        if (sent == -1) 
            return -1;
        bytes_sent += sent;
        bytes_unsent -= sent;
    }
    
    return bytes_sent;
}
