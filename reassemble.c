/* mimocat -- netcat that distributes stdin/stdout over multiple TCP streams

   Copyright (c) 2013 Susan Werner <heinousbutch@gmail.com>
   
   reassemble.c -- handle reordering of cells and data stream reassembly 
 */

#include "mimocat.h"


/* the way this code works is that it keeps two structures around -- one with 
   information on what's already been reassembled and one with information on
   things which haven't been reassembled yet
   
   the former structure is called reassembly_state, and the latter structure is
   just simply the unpacked_cell structure that unpack_cell() writes to.

   unpack_cell() doesn't handle anything to do with either the payload nor the
   unpacked_cell->next pointer, it's up to us to do so here.
 
 
 */







typedef struct reassembly_state {
    
} reassembly_state_t;




