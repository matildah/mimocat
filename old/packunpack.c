/* mimocat -- netcat that distributes stdin/stdout over multiple TCP streams

   Copyright (c) 2013 Susan Werner <heinousbutch@gmail.com>
   
   writeout.c -- handle writing cells in and out
 */


#include "mimocat.h"


/* pack_cell -- take a cell_hdr struct and some data and pack it into a blob
   of data fit for sending on the network */

void pack_cell(cell_hdr_t *source, uint8_t *payload, packed_cell_t *dest)
{
   uint32_t  seqprime, lenprime;
   /* things go in network byte order */ 
   seqprime = htonl(source->seq);
   lenprime = htonl(source->payload_len);
   /* ...and into the right places */
   memcpy((dest->data),           &(source->type), 1);
   memcpy((dest->data) + 1,       &seqprime,       4);
   memcpy((dest->data) + 5,       &lenprime,       4);
   memcpy((dest->data) + HDR_LEN, payload,         source->payload_len);

   /* ...and we put how much data we wrote in the appropriate place */
   dest->data_len = source->payload_len + HDR_LEN;
}



/* unpack_cell -- takes a blob of data, size bytes long, pointed to by source
   and extracts and unpacks the cell header (putting it in the dest pointer,
   as well as returning a pointer to the end of the cell+payload that was
   processed. 
   
   the caller of this function needs to handle the whole "copy the payload to 
   an appropriate buffer" thing -- unpack_cell does not do anything wth the 
   payload besides return a pointer to its last byte.

   the chunk of data pointed to by source can contain stuff after the end of
   the cell but *must* start with a cell_hdr.

   if it returns NULL, this means that no valid cells were found.

   */


uint8_t *unpack_cell(uint8_t *source, size_t size, unpacked_cell_t *dest)
{
    uint32_t seq, payload_len;
    uint8_t type;


    if(size < HDR_LEN) /* too short to even contain a single cell header */
    {
        return NULL;
    }
    memcpy(&type, source, 1);
    memcpy(&seq, (source + 1), 4);
    memcpy(&payload_len, (source + 5), 4);
    
    if (type != DATA_TYPE && type != CMD_TYPE)
    {
        return NULL;
    }

    seq = ntohl(seq);                 /* byte order conversion */
    payload_len = ntohl(payload_len);

    if (payload_len > (size - HDR_LEN)) /* we know size >= HDR_LEN so we
                                         won't have an integer overflow
                                         here */
    {
        return NULL;                  /* packet's header will have us reading
                                         past the buffer's end, can't let that
                                         happen */
    }
    dest->hdr.type=type;
    dest->hdr.seq=seq;
    dest->hdr.payload_len=payload_len;

    /* i commented the step where we allocate a buffer for the payload and 
       copy the payload to that buffer, that is better done by whoever calls
       unpack_cell */

/*  dest->payload=malloc(payload_len);               
    memcpy(dest->payload, (source+HDR_LEN), payload_len);
*/
    return source + HDR_LEN + (payload_len - 1); /* this is a pointer to the 
                                                  last byte of the payload, 
                                                  inside the original buffer 
                                                  we got given */

    /* it is up to our caller to copy the data from (source + HDR_LEN) to 
       (source + HDR_LEN + payload_len) (the return value) if they, for some 
       odd reason, want the payload somewhere */

}



/*
void main ()
{

    cell_hdr_t *from = malloc(sizeof(cell_hdr_t));
    packed_cell_t *to= malloc(sizeof(packed_cell_t));
    uint8_t *end;
    unpacked_cell_t *check= malloc(sizeof(unpacked_cell_t));
    uint8_t payload [] = {0x41,0x41,0x41,0x41,0x41,0x41,0x00};
    uint8_t *payload_m = malloc(7);
    memcpy(payload_m,&payload,7);
    printf("%s",payload_m);
    from->payload_len=7;
    from->seq=0xfeedface;
    from->type=0xFE;
    to->data=malloc(HDR_LEN + from->payload_len);

    pack_cell(from, payload_m, to);

    end = unpack_cell(to->data, to->data_len, check);
    assert(check->hdr.payload_len == from->payload_len);
    assert(check->hdr.seq == from->seq);
    assert(check->hdr.type == from->type);

    free(check->payload);
    free(check);
    free(from);
    free(to->data);
    free(to);
    free(payload_m);
}


*/
