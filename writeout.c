/* mimocat -- netcat that distributes stdin/stdout over multiple TCP streams

   Copyright (c) 2013 Susan Werner <heinousbutch@gmail.com>
   
   writeout.c -- handle writing cells in and out
 */

#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct cell_hdr {       /* header of a cell */
    uint8_t  type;      /* cell type -- command or data?*/
    uint32_t seq;       /* sequence number */
    uint32_t payload_len;       /* length of the payload, NOT including the header */
} cell_hdr_t;

#define DATA_TYPE   0xFE
#define CMD_TYPE    0xED

typedef struct packed_cell {    /* packed cell, ready to send */
    uint32_t data_len;       /* length of the data */
    uint8_t *data;      /* pointer to data */
} packed_cell_t;


typedef struct unpacked_cell {  /* how we store cells after unpacking them but
                                  prior to reassembling them */
    cell_hdr_t hdr;             /* a copy of the header */
    uint8_t  *payload;          /* a pointer to the payload */
                                /* no need to have a separate variable in this 
                                   structure for the length of the payload, as
                                   it's already included in hdr */
} unpacked_cell_t;

#define HDR_LEN (1+4+4) /* length of the header in bytes*/


/* pack_cell -- take a cell_hdr struct and some data and pack it into a blob
   of data fit for sending on the network */

void pack_cell(cell_hdr_t *source, uint8_t *payload, packed_cell_t *dest)
{
   uint32_t  seqprime, lenprime;
   seqprime = htonl(source->seq);
   lenprime = htonl(source->payload_len);
   memcpy(dest->data, &(source->type), 1);
   memcpy((dest->data) + 1, &seqprime, 4);
   memcpy((dest->data) + 5, &lenprime, 4);
   memcpy((dest->data) + 9, payload, source->payload_len);
   dest->data_len=source->payload_len + HDR_LEN;
}



/* unpack_cell -- takes a blob of data, size bytes long, pointed to by source
   and extracts and unpacks the cell header (putting it in the dest pointer,
   as well as returning a pointer to the end of the cell+payload that was
   processed. 

   the chunk of data pointed to by source can contain stuff after the end of
   the cell but *must* start with a cell_hdr.

   if it returns NULL, this means that no cells were found.

   */


uint8_t *unpack_cell(uint8_t *source, uint32_t size, unpacked_cell_t *dest)
{
    uint32_t seq, payload_len;
    uint8_t type;


    if(size < HDR_LEN) /* too short to even contain a single cell header */
    {
        return NULL;
    }
    memcpy(&type, source, 1);
    memcpy(&seq, (source +1), 4);
    memcpy(&payload_len, (source +5), 4);
    
    if (type != DATA_TYPE && type != CMD_TYPE)
    {
        return NULL;
    }

    seq = ntohl(seq);
    payload_len= ntohl(payload_len);

    if (payload_len + HDR_LEN > size)
    {
        return NULL;
    }

    dest->hdr.type=type;
    dest->hdr.seq=seq;
    dest->hdr.payload_len=payload_len;
    dest->payload=malloc(payload_len);
    memcpy(dest->payload, (source+9), payload_len);

    return source + HDR_LEN + payload_len;

}




void main ()
{
    cell_hdr_t *from = malloc(sizeof(cell_hdr_t));
    packed_cell_t *to= malloc(sizeof(packed_cell_t));
    uint8_t *end;
    unpacked_cell_t *check= malloc(sizeof(unpacked_cell_t));
    uint8_t payload [] = {0x41,0x41,0x41,0x41,0x41,0X41,0x00};
    uint8_t *payload_m = malloc(7);
    memcpy(payload_m,&payload,7);
    printf("%s",payload_m);
    from->payload_len=7;
    from->seq=0xfeedface;
    from->type=0xFE;
    to->data=malloc(HDR_LEN + from->payload_len);

    pack_cell(from, payload_m, to);

    end = unpack_cell(to->data, to->data_len, check);
    free(check->payload);
    free(check);
    free(from);
    free(to->data);
    free(to);
    free(payload_m);
}



