/* this is mimocat, implemented with a control TCP connection for transferring
   state
 */
#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define NUMFDS 256 /* maximum number of simultanous data connections */

/* this struct contains information on a chunk of data that was sent over one 
   of our data connections. this is a struct that we will pack and send over
   the control connection.

   if the XXXXX'd bytes represent a chunk (here, it's 3 bytes long), the 
   offsets begin_off and end_off are set up as follows

               /- begin_off = 2
               | 
               v
   +0    +1    +2    +3    +4    +5   +6     +7
[-----|-----|XXXXX|XXXXX|XXXXX|-----|-----|-----]
                           ^
                           |
                           \- end_off = 4 

*/

typedef struct chunk_hdr {
    uint8_t index; /* which connection we sent this over */
    uint32_t begin_off;  /* what was the offset of the first byte of this chunk */
    uint32_t end_off;    /* what was the offset of the last byte we sent */
    uint32_t seq;  /* this chunk's sequence number */
} CHUNK_HDR;

#define CHUNK_HDR_LEN (1+4+4+4)

/* packed struct, ready to send */
typedef struct packed_chunk {
    uint8_t *data;
    size_t  len;
} PACKED_CHUNK;

typedef struct unpacked_chunk {
    CHUNK_HDR info; /* chunk information header */
    struct unpacked_chunk *next; /* pointer to the next header */
} UNPACKED_CHUNK;


/* holds the association between the file descriptors for our data connections
   and the index numbers -- the index numbers are the same for us and our peer, 
   the fd numbers are likely different */

typedef struct fd_array {
    int numfds;                /* number of file descriptors we have stored */

    int fds [NUMFDS];          /* holds file descriptors, indexed in an arbitrary 
                                  order from 0 (inclusive) to numfds-1 (inclusive)
                                */
    
    uint8_t indices [NUMFDS];  /* holds index numbers, indexed in parallel with 
                                  the fd array */
    uint32_t bytes [NUMFDS];   /* if we are the sender, this tells us how many
                                  bytes have been written to the socket in 
                                  question. if we are the receiver, this is 
                                  where we keep track of how many bytes we've 
                                  read over this socket */

    int lastidx;               /* index into arrays {fds, indices, bytes} of 
                                  the last connection we sent data down */
    uint32_t lastseq;          /* the last sequence number we used */
} FD_ARRAY;

