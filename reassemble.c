/* mimocat -- netcat that distributes stdin/stdout over multiple TCP streams

   Copyright (c) 2013 Susan Werner <heinousbutch@gmail.com>
   
   reassemble.c -- handle reordering of cells and data stream reassembly 
 */

#include "mimocat.h"



/* there are two different steps in taking data from more than one network 
   stream, reassembling cells, and reordering them into one stdout stream.

   1) for each input stream, we convert non-delimited data into cells, and 
   unpack those cells, and add them to the linked list.

   2) look at the linked list and reassemble cells (in order!) into one output 
   data stream.


   step one needs to have one instance of state per each network stream, while 
   step two works on one linked list (for all the incoming network streams).

 */


#define INITIALBUFFER (1024*8) /* initial length of the reassembly buffer. 
                                this is totally arbitrary */

/* this is the state for step one. there is one instance of this structure for
   each input stream */

typedef struct reassembly_state {

    uint8_t *start;                 /* the byte *after* the last byte of the last 
                                    cell we fully processed. this is a pointer within the 
                                    buffer called "incomplete" */

    uint8_t *incomplete;            /* a buffer where we keep data that does 
                                       not compose a full cell */
    size_t incomplete_len;          /* current size of that buffer */


} reassembly_state_t;



/* this is the state for step 2. there is one instance of this structure per 
   output stream (so just 1 instance assuming we're just writing to stdout) */

typedef struct reordering_state{
    unpacked_cell_t *head;            /* the head of the linked list where we 
                                         store not-yet-processed cells */
    uint32_t last;                    /* the sequence number of the last cell 
                                         we processed (aka, the sequence number
                                         of the last cell that got removed from
                                         the linked list */
} reordering_state_t;





reassembly_state_t * initialize_reass()
{

    reassembly_state_t * state=malloc(sizeof(reassembly_state_t));
    assert(state != NULL);
    state->incomplete = malloc(INITIALBUFFER);
    assert(state->incomplete != NULL);
    state->start= state->incomplete;
    state->incomplete_len = INITIALBUFFER;

}

/* takes a blob of data (with no specific requirements on it) and adds it to
   the end of the incomplete buffer*/
int push_data(uint8_t *data, size_t len, reassembly_state_t *state)
{
}

