/* the functions in this file handle the whole "sending data out" thing */

#include "state.h"

/* this function sends a chunk of data down the next appropriate fd and a
   corresponding chunk header down the control connection */
int send_chunk(FD_ARRAY *fdstate, uint8_t *data, size_t len)
{
    CHUNK_HDR ourheader;
    PACKED_CHUNK packedchunk;
    int fd;
    assert(fdstate -> numfds > 0); /* make sure we have FDs ready */


    fd = fdstate->fds[fdstate->nextidx]; /* which FD we use */
    
    /* we prepare the chunk header */
    ourheader.index = fdstate->fds[fdstate->nextidx];
    ourheader.begin_off = fdstate->bytes[fdstate->nextidx];
    ourheader.end_off = ourheader.begin_off + len;
    ourheader.seq = fdstate->nextseq;

    /* and allocate space for the packed version of it */
    packedchunk.data = malloc(CHUNK_HDR_LEN);
    assert(packedchunk.data != NULL);
    
    /* and we pack it */
    pack_header(&ourheader, &packedchunk);
    /* and we send it down the control connection */
    if(send_all(fdstate->controlfd, packedchunk.data, packedchunk.len, 0) == -1)
    {
        perror("send error in control connection");
        exit(-1);
    }
    /* it's sent, so we don't need its packed version */
    free(packedchunk.data);

    /* ...and we send the corresponding data */
    if(send_all(fd, data, len, 0) == -1)
    { 
        perror("send error in data connection");
        exit(-1);
    }

    /* here we update fdstate */
    fdstate->bytes[fdstate->nextidx] += len; /* how many bytes we sent down that fd*/
    /* we update nextidx while making sure it doesn't get too big */
    fdstate->nextidx++;
    if (fdstate->nextidx > (fdstate->numfds - 1))
    {
        fdstate->nextidx = 0;
    }


    fdstate->nextseq++;
}


