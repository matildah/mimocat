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
    uint32_t len;       /* length of the payload */
} cell_hdr_t;

typedef struct packed_cell {    /* packed cell, ready to send */
    uint32_t len;       /* length of the data */
    uint8_t *data;      /* pointer to data */
} packed_cell_t;


#define HDR_LEN (1+4+4) /* length of the header in bytes*/


/* pack_cell -- take a cell_hdr struct and some data and pack it into a blob
   of data fit for sending on the network */

void pack_cell(cell_hdr_t *source, uint8_t *payload, packed_cell_t *dest)
{
   uint32_t  seqprime, lenprime;
   seqprime = htonl(source->seq);
   lenprime = htonl(source->len);
   memcpy(dest->data, &(source->type), 1);
   memcpy((dest->data) + 1, &seqprime, 4);
   memcpy((dest->data) + 5, &lenprime, 4);
   memcpy((dest->data) + 9, payload, source->len);
   dest->len=source->len + HDR_LEN;
}


void main ()
{
    packed_cell_t *to= malloc(sizeof(packed_cell_t));
    cell_hdr_t *from = malloc(sizeof(cell_hdr_t));
    uint8_t payload [] = {0x41,0x41,0x41,0x41,0x41,0X41,0x00};
    uint8_t *payload_m = malloc(7);
    memcpy(payload_m,&payload,7);
    printf("%s",payload_m);
    from->len=7;
    from->seq=0xfeedface;
    from->type=69;
    to->data=malloc(HDR_LEN + from->len);
    pack_cell(from, payload_m, to);
    free(from);
    free(to->data);
    free(to);
    free(payload_m);
}



