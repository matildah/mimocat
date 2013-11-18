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


#define INITIALBUFFER (1) /* initial length of the reassembly buffer. 
                                this is totally arbitrary */

/* this is the state for step one. there is one instance of this structure for
   each input stream */

/* this structure is used to manage a buffer called "incomplete". We maintain
   two pointers within this buffer -- one called writepos, one called readpos.

   writepos points to the byte AFTER the last byte of the last chunk of data 
   we wrote to the buffer. writepos is only to be modified by code that adds
   data to the buffer. adding x bytes to the buffer means writepos will be 
   incremented by x.

   readpos points to the initial byte of valid cell data. readpos 
   is only to be modified by code that takes valid cells out of the buffer.


   if we ever have writepos == readpos, this means there are no data in the 
   buffer and that we are free to set both writepos and readpos to point at
   the beginning of the buffer (to prevent it from growing indefinitely big)
    

   if bytes with valid data are shown by XXXXX, this is how the pointers 
   are set up: 

       /- incomplete                 (incomplete + incomplete_len)
       |                                         |
       v                                         v
       +0    +1    +2    +3    +4    +5   +6     +7
    [-----|-----|XXXXX|XXXXX|XXXXX|-----|-----|-----]
                   ^                 ^
                   \ readpos         \ writepos

*/

typedef struct reassembly_state {
    uint8_t *writepos;     /* pointer within incomplete */
    uint8_t *readpos;      /* pointer within incomplete */            
    uint8_t *incomplete;   /* the reassembly buffer */
    size_t incomplete_len; /* and its length */
} reassembly_state_t;



/* this is the state for step 2. there is one instance of this structure per 
   output stream (so just 1 instance assuming we're just writing to stdout) */

typedef struct reordering_state{
    unpacked_cell_t *head;            /* the head of the linked list where we 
                                         store not-yet-processed cells */
    uint32_t last_seq;                /* the sequence number of the last cell 
                                         we processed (aka, the sequence number
                                         of the last cell that got removed from
                                         the linked list */
} reordering_state_t;







reassembly_state_t * initialize_reass()
{
    reassembly_state_t * state=malloc(sizeof(reassembly_state_t));
    assert(state != NULL);
    state->incomplete = malloc(sizeof(uint8_t) * INITIALBUFFER);
    assert(state->incomplete != NULL);
    state->writepos = state->incomplete;
    state->readpos = state->incomplete;
    state->incomplete_len = INITIALBUFFER;
    return state;
}

/* takes a blob of data (with no specific requirements on it) and adds it to
   the end of the incomplete buffer*/
void push_data(uint8_t *data, size_t len, reassembly_state_t *state)
{
    size_t bytes_left; /* how many bytes are available inside the buffer */
    assert (state->writepos >= state->incomplete);
    assert (state->incomplete_len >= (state->writepos - state->incomplete));

    bytes_left = state->incomplete_len - (state->writepos - state->incomplete);
    
    if(bytes_left <= len)
    { /* ok, we need to grow our buffer to accomodate the incoming data */
       
       size_t grow = len; /* we can grow it by how much we want, but it needs
                               to be at least big enough to hold the new data */

       size_t offset_w; 
       size_t offset_r;

       /* we need to convert the pointers to things within the buffers to 
          offsets because realloc might change where the buffer begins */
       offset_w = state->writepos - state->incomplete;
       offset_r = state->readpos - state->incomplete;
          

       uint8_t *newbuf = realloc(state->incomplete, state->incomplete_len + grow );
       assert (newbuf != NULL);

       state->incomplete = newbuf; /* ok, realloc worked */

       state->incomplete_len += grow; /* keep track of the current buffer size */

       state->writepos = state->incomplete + offset_w; 
       state->readpos = state->incomplete + offset_r; 
    }
    
    memcpy(state->writepos, data, len);
    state->writepos += len;

}



/* pop_cell takes in a reassembly_state structure and looks into the 
   incomplete buffer for completed cells. It returns an unpacked_cell structure
   as well as advancing the readpos pointer appropriately 
   */

unpacked_cell_t * pop_cell(reassembly_state_t *state) 
{
    unpacked_cell_t *out;
    uint8_t *endofcell;
    out = malloc(sizeof(unpacked_cell_t));
    assert (out != NULL);
    assert (state->writepos != NULL);
    assert (state->readpos  != NULL);

    if(state->writepos == state->readpos)
    {
        /* no bytes for us to read here, might as well reset these pointers
           back to the start of the buffer */
        state->writepos = state->incomplete;
        state->readpos = state->incomplete;
        free(out);
        return NULL;
    }

    /* where we start reading should be before where we'll end reading 
     otherwise horrible things may happen */
    assert(state->writepos > state->readpos);
    
    endofcell = unpack_cell(state->readpos, state->writepos - state->readpos, out);

    if(endofcell == NULL)
    {
        free(out);
        return NULL;
    }
    out->payload = malloc(out->hdr.payload_len);
    assert (out->payload != NULL);
    memcpy(out->payload, (state->readpos + HDR_LEN), out->hdr.payload_len);

    state->readpos = endofcell + 1;
    return out;

}


reordering_state_t * initialize_reorder()
{
    reordering_state_t *state = malloc(sizeof(reordering_state_t));
    assert(state != NULL);
    state->last_seq = 0;
    state->head = NULL;
    return state;
}

void reorder_add(unpacked_cell_t *newcell, reordering_state_t *state)
{
    unpacked_cell_t *tmp;
    assert(newcell != NULL);
    assert(state != NULL);
    if (state->head == NULL)
    {
        state->head = newcell;
        newcell->next=NULL;
        return;
    }

    
    for (tmp = state->head; tmp->next != NULL; tmp=tmp->next)
    {
    }
    
    assert(tmp != NULL);
    tmp->next = newcell;
    newcell->next=NULL;
}


size_t reorder_pop(uint8_t *data, reordering_state_t *state)
{
}

int main() {
    reassembly_state_t *state = initialize_reass();
    unpacked_cell_t *foobar, *b, *c;
    cell_hdr_t *hdr = malloc(sizeof(cell_hdr_t));
    packed_cell_t *dest = malloc(sizeof(packed_cell_t));
    reordering_state_t *reorder = malloc(sizeof(reordering_state_t));
    assert(dest != NULL);
    assert(hdr != NULL);

    dest->data = malloc(7 + HDR_LEN);
    uint8_t data [] = {0x41,0x41,0x41,0x41,0x41,0x41,0x42};
    
    
    hdr->seq=0xabadbeef;
    hdr->payload_len=7;
    hdr->type=DATA_TYPE;

    pack_cell(hdr, data, dest);
    push_data(dest->data, dest->data_len, state);
    data[0] = 0x43;
    data[1] = 0x44;

    hdr->seq=0xabadbeee;
    hdr->payload_len=2;
    hdr->type=DATA_TYPE;

    pack_cell(hdr, data, dest);
    push_data(dest->data, dest->data_len, state);

    foobar = pop_cell(state);    
    b = pop_cell(state);    
    c = pop_cell(state);

    reorder = initialize_reorder();
    reorder_add(foobar, reorder);
    reorder_add(b, reorder);


    free(foobar->payload);
    free(foobar);
    free(hdr);
    free(dest->data);
    free(dest);
    free(state->incomplete);
    free(b);
    free(state);
    return 0;

    
}

