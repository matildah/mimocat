/* the functions in this file handle the whole "sending data out" thing */

#include "state.h"

/* this function sends a chunk of data down the next appropriate fd and a
   corresponding chunk header down the control connection */
void send_chunk(FD_ARRAY *fdstate, uint8_t *data, size_t len)
{
    CHUNK_HDR ourheader;
    PACKED_CHUNK packedchunk;
    int fd;
    assert(fdstate -> numfds > 0); /* make sure we have FDs ready */


    fd = fdstate->fds[fdstate->nextidx]; /* which FD we use */
    
    /* we prepare the chunk header */
    ourheader.index = fdstate->indices[fdstate->nextidx];
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
    /* increment the sequence number */
    fdstate->nextseq++;
}

/* creates and connects sockets for all the data connections */
FD_ARRAY* data_sockets(HOSTS_PORTS *hp)
{
    FD_ARRAY *fdarray;
    int fd, rval, i;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    fdarray = malloc(sizeof(FD_ARRAY));
    assert(fdarray != NULL);
    assert(hp != NULL);

    fdarray->numfds = hp->numpairs;
    fdarray->nextidx = 0;
    fdarray->nextseq = 0;

    /* let's do fun stuff with getaddrinfo now */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* either ipv4 or ipv6, we don't care */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;


    for(i = 0; i < hp->numpairs; i++)
    {
        rval = getaddrinfo(hp->nodes[i], hp->ports[i], &hints, &result);
        if (rval != 0)
        {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rval));
            exit(1);
        }
        /* now we iterate over the lists of results that getaddrinfo returned
           until we can successfully make a socket and connect with it */
        for (rp = result; rp != NULL; rp = rp->ai_next)
        {
            fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (fd == -1)
            {
                /* the socket making failed, so we need to do a different 
                   address */
                continue;
            }

            /* we made a socket, now we try to connect */
            if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
            {
                break;  /* we successfully connected, let's exit this loop */
            }
            
            close(fd); /* making the socket worked but connect() failed so we 
                          close this socket */
        }
        if (rp == NULL) /* no address worked */
        {
            fprintf(stderr, "Could not connect to %s:%s\n",hp->nodes[i], hp->ports[i]);
            exit(1);
        }
        freeaddrinfo(result);

        /* we now have a socket open, now let's store info about it in our 
           FD_ARRAY */
        fdarray->fds[i] = fd;
        fdarray->indices[i] = i;
        fdarray->bytes[i] = 0;

    }
    return fdarray;
}

/* initializes the control socket */
void control_socket(FD_ARRAY *fdarray, char *node, char *port)
{
    int fd, rval;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    assert(fdarray != NULL);

    /* let's do fun stuff with getaddrinfo now */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* either ipv4 or ipv6, we don't care */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    rval = getaddrinfo(node, port, &hints, &result);
    if (rval != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rval));
        exit(1);
    }
    /* now we iterate over the lists of results that getaddrinfo returned
       until we can successfully make a socket and connect with it */
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1)
        {
            /* the socket making failed, so we need to do a different 
               address */
            continue;
        }

        /* we made a socket, now we try to connect */
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
        {
            break;  /* we successfully connected, let's exit this loop */
        }

        close(fd); /* making the socket worked but connect() failed so we 
                      close this socket */
    }
    if (rp == NULL) /* no address worked */
    {
        fprintf(stderr, "Could not connect to %s:%s\n",node, port);
        exit(1);
    }
    freeaddrinfo(result);

    /* we now have a socket open, now let's store info about it in our 
       FD_ARRAY */
    fdarray->controlfd = fd;    
}


/* sends the index of each data connection down the corresponding data 
   connection so our peer can match against the indices we send over the
   control channel */

void initial_data(FD_ARRAY *fd)
{
    assert (fd != NULL);
    int i;
    for (i = 0; i < fd->numfds; i++)
    {
        if(send_all(fd->fds[i], &fd->indices[i], 1, 0) == -1)
        {
            perror("send_all");
            exit(-1);
        }
    }
}


/* loops, copying data from the fdfrom file descriptor and calls send_chunk
   on each chunk of data */
void mainloop(int fdfrom, FD_ARRAY *fd)
{
    ssize_t read_bytes;
    uint8_t buf [BLOCKSIZE];

    while (1)
    {
        read_bytes = read(fdfrom, &buf, BLOCKSIZE);
        if (read_bytes == -1)
        {
            perror("read error");
        }
        
        if (read_bytes == 0)
        {
            break;
        }
        send_chunk(fd, buf, read_bytes);
    }
}



int main(int argc, char* argv[])
{
    HOSTS_PORTS hp;
    FD_ARRAY *fd;
    int i;
    if (argc < 5)
    {
        printf("usage: %s controlhost controlport datahost_1 dataport_1 [datahost_2 dataport_2 ...]\n", argv[0]);
        exit(10);
    }


    hp.numpairs = 0;
    for (i = 0; 2*i + 4 < argc ;i++)
    {
        hp.nodes[i] = argv[2*i+3];
        hp.ports[i] = argv[2*i+4];
        hp.numpairs++;
    }

    assert(hp.numpairs <= NUMFDS);

    fd = data_sockets(&hp);
    initial_data(fd);
    control_socket(fd, argv[1], argv[2]);
    mainloop(0, fd);
    for (i = 0; i < fd->numfds; i++)
    {
        close(fd->fds[i]);
    }

    close(fd->controlfd);
    free(fd);
    return 0;
}

