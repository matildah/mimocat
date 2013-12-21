/* the functions in this file handle the whole "sending data out" thing */

#include "state.h"

/* this function sends a chunk of data down the next appropriate fd */
int send_chunk(FD_ARRAY *fdstate, uint8_t data, size_t len)
{
    CHUNK_HDR ourheader;
    PACKED_CHUNK packedchunk;
    int fd;
    uint8_t *buf;
    size_t buflen;
    assert(fdstate -> numfds != 0);

    fdstate->lastidx++;
    if(fdstate->lastidx > fdstate->numfds - 1)
    {
        fdstate->lastidx = 0;
    }

    buflen = len + CHUNK_HDR_LEN;
    assert(buflen > len && buflen > CHUNK_HDR_LEN);
    buf = malloc(buflen);
    assert (buf != NULL);

    fd = fdstate->fds[fdstate->lastidx];

    ourheader.index = fdstate->fds[fdstate->lastidx];
    ourheader.begin_off = fdstate->bytes[fdstate->lastidx];
    ourheader.end_off = ourheader.begin_off + len;
    ourheader.seq = fdstate->lastseq++;


    packedchunk.data = malloc(CHUNK_HDR_LEN);
    assert(packedchunk.data != NULL);
    
    pack_header(&ourheader, &packedchunk);

    memcpy(buf,                   packedchunk.data, packedchunk.len);
    memcpy(buf + packedchunk.len, data,             len);
    free(packedchunk.data);

}


