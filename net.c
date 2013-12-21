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


