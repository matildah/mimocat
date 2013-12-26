/* copyright (c) 2013 Matilda Helou <hypernymy@gmail.com> */
/* this file is where we have the functions that handle receiving */

#include "state.h"


/* creates a bunch of listening sockets for the data connections */
FD_ARRAY* data_listeners(HOSTS_PORTS *hp)
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
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;


    for(i = 0; i < hp->numpairs; i++)
    {
        rval = getaddrinfo(NULL, hp->ports[i], &hints, &result);
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

            /* we made a socket, now we try to bind  */
            if (bind(fd, rp->ai_addr, rp->ai_addrlen) != -1)
            {
                break;  /* we successfully bound, let's exit this loop */
            }
            
            close(fd); /* making the socket worked but bind() failed so we 
                          close this socket */
        }
        if (rp == NULL) /* no address worked */
        {
            fprintf(stderr, "Could not bind to %s\n", hp->ports[i]);
            exit(1);
        }
        freeaddrinfo(result);
        listen(fd,1);
        /* we now have a socket open, now let's store info about it in our 
           FD_ARRAY */
        fdarray->fds[i] = fd;
        fdarray->bytes[i] = 0;

    }
    return fdarray;
}


/* receives the index of each data connection through the corresponding data 
   connection so we can match against the indices that our peer sends over 
   the control channel */

void r_initial_data(FD_ARRAY *fd)
{
    assert (fd != NULL);
    int i;
    for (i = 0; i < fd->numfds; i++)
    {
        if(recv_all(fd->fds[i], &fd->indices[i], 1, 0) != 1)
        {
            perror("recv_all");
            exit(-1);
        }
    }
}


void control_listener(FD_ARRAY *fdarray, char *port)
{
    int fd, rval, i;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    assert(fdarray != NULL);

    /* let's do fun stuff with getaddrinfo now */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* either ipv4 or ipv6, we don't care */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;


    rval = getaddrinfo(NULL, port, &hints, &result);
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

        /* we made a socket, now we try to bind  */
        if (bind(fd, rp->ai_addr, rp->ai_addrlen) != -1)
        {
            break;  /* we successfully bound, let's exit this loop */
        }

        close(fd); /* making the socket worked but bind() failed so we 
                      close this socket */
    }
    if (rp == NULL) /* no address worked */
    {
        fprintf(stderr, "Could not bind to %s\n", port);
        exit(1);
    }
    freeaddrinfo(result);
    listen(fd,1);
    /* we now have a socket open, now let's store info about it in our 
       FD_ARRAY */
    fdarray->controlfd = fd;
}


/* run accept() on our fds */

void accept_all(FD_ARRAY *fd)
{
    int status, i;
    status = accept(fd->controlfd, NULL, NULL);
    if(status == -1)
    {
        perror("accept error on control connection");
        exit (-1);
    }
    close(fd->controlfd);
    fd->controlfd = status;


    for(i = 0; i < fd->numfds; i++)
    {
        status = accept(fd->fds[i], NULL, NULL);
        if(status == -1)
        {
            perror("accept error on data connection");
            exit (-1);
        }
        close(fd->fds[i]);
        fd->fds[i] = status;
    }
}


/* this is the part where we loop over receiving stuff over the control
   connection, and then read from the appropriate data connection */

void main_loop(int fdout, FD_ARRAY *fd)
{
    ssize_t status;
    int i, index, datafd;
    uint8_t * buf;
    uint8_t * hdrbuf;
    uint32_t chunksize;
    UNPACKED_CHUNK unpacked_hdr;
    hdrbuf = malloc(CHUNK_HDR_LEN);

    while (1)
    {
        status = recv_all(fd->controlfd, hdrbuf, CHUNK_HDR_LEN, 0);
        if (status == -1)
        {
            perror("error in recv'ing over control connection");
            exit (-1);
        }
        if (status == 0)
        {
            return;
        }

        unpack_header(hdrbuf, CHUNK_HDR_LEN, &unpacked_hdr);


        for (i = 0; i < fd->numfds; i++)
        {
            if (fd->indices[i] == unpacked_hdr.info.index)
            {
                datafd = fd->fds[i];
                break;
            }
        }
        chunksize = unpacked_hdr.info.end_off - unpacked_hdr.info.begin_off;
        buf = malloc(chunksize);
        assert(buf != NULL);

        status = recv_all(datafd, buf, chunksize, 0);

        if (status == -1)
        {
            perror("error in recv'ing over data connection");
            exit(-2);
        }
        if (status == 0)
        {
            return;
        }

        status = write_all(fdout, buf, chunksize);
        
        if(status != chunksize)
        {
            perror("error in sending on output");
            exit(-3);
        }
        free(buf); 


    }
}

int main(int argc, char *argv[])
{
    HOSTS_PORTS hp;
    FD_ARRAY *fd;
    int i;

    if(argc < 3)
    {
        printf("usage: %s controlport dataport_1 [dataport_2] [dataport_3] ... [dataport_n]\n",argv[0]);
        exit(-1);
    }
    hp.numpairs = 0;

    for(i = 0 ; i+2 < argc ; i++)
    {
        hp.ports[i] = argv[i+2];
        hp.numpairs++;
    }
    assert(hp.numpairs <= NUMFDS);
    fd = data_listeners(&hp);
    control_listener(fd, argv[1]);
    accept_all(fd);
    r_initial_data(fd);

    main_loop(1, fd);
    for (i = 0; i < fd->numfds; i++)
    {
            close(fd->fds[i]);
    }

    close(fd->controlfd);
    free(fd);

}
