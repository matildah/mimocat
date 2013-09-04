/* mimocat -- netcat that distributes stdin/stdout over multiple TCP streams

   Copyright (c) 2013 Susan Werner <heinousbutch@gmail.com>
   
   mimocat.h -- definitions of structures and also the files we want to include
  
 */

#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef struct cell_hdr {       /* header of a cell */
    uint8_t  type;      /* cell type -- command or data?*/
    uint32_t seq;       /* sequence number */
    uint32_t payload_len;       /* length of the payload, NOT including the header */
} cell_hdr_t;

#define HDR_LEN (1+4+4) /* length of the header in bytes*/

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



