/* this is mimocat, implemented with a control TCP connection for transferring
   state
 */
#include <stdint.h>
#include <stdlib.h>



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

typedef struct chunk_info {
    uint8_t index; /* which connection we sent this over */
    uint32_t begin_off;  /* what was the offset of the first byte of this chunk */
    uint32_t end_off;    /* what was the offset of the last byte we sent */
    uint32_t seq;  /* this chunk's sequence number */
} chunk_t;


/* packed struct, ready to send */
typedef struct packed_chunk {
    uint8_t *data;
    size_t  len;
} packed_chunk_t;

typedef struct unpacked_chunk {
    chunk_t info; /* chunk information header */
    struct unpacked_chunk *next; /* pointer to the next header */
} unpacked_chunk_t;


/* holds the association between the file descriptors for our data connections
   and the index numbers -- the index numbers are the same for us and our peer, 
   the fd numbers are likely different */

typedef struct fd_array {
    int fds [256];          /* holds file descriptors, indexed arbitrarily */
    uint8_t indices [256];  /* holds index numbers, indexed in parallel with the fd
                               array */
    uint8_t numfds;         /* number of file descriptors we have stored */
} fd_array_t;



int 
