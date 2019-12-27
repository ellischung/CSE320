#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "protocol.h"
#include "debug.h"
#include "csapp.h"
#include "server.h"
#include "exchange.h"
#include "trader.h"
#include "client_registry.h"

int proto_send_packet(int fd, BRS_PACKET_HEADER *hdr, void *payload) {
    // save size of header
    uint32_t hdr_length = sizeof(*hdr);

    // ALREADY IN NETWORK ORDER, UNNECESSARY TO CONVERT BELOW

    // call write
    int rval = write(fd, hdr, hdr_length);
    //debug("write rval: %d", rval);

    // check for short counts and re-execute write if there are short counts
    while (rval) {
        if (rval < 0) {
            errno = rval;
            return -1;
        }
        hdr_length = hdr_length - rval;
        if (hdr_length == 0) {
            break;
        }
        rval = write(fd, hdr, hdr_length);
    }

    // save payload length, if any (onverted from network)
    uint16_t pl_length = ntohs(hdr->size);

    // check if payload size is non-zero
    if (pl_length != 0) {
        // call write
        int rval_for_pl = write(fd, payload, pl_length);
        //debug("write rval again for pl: %d", rval_for_pl);

        // check for short counts and re-execute write if there are short counts
        while (rval_for_pl) {
            if (rval_for_pl < 0) {
                errno = rval_for_pl;
                return -1;
            }
            pl_length = pl_length - rval_for_pl;
            rval_for_pl = write(fd, payload, pl_length);
        }
    }

    // no errors, exit successfully
    return 0;
}

int proto_recv_packet(int fd, BRS_PACKET_HEADER *hdr, void **payloadp) {
    // save the size of the packet header
    uint32_t hdr_length = sizeof(*hdr);
    //debug("hdr length: %d", hdr_length);

    // call read
    int rval = read(fd, hdr, hdr_length);
    //debug("read rval: %d", rval);

    // check for short counts and re-execute read if there are short counts
    while (rval) {
        if (rval < 0) {
            errno = rval;
            return -1;
        }
        hdr_length = hdr_length - rval;
        rval = read(fd, hdr, hdr_length);
    }

    // convert the retrieved packet to host order
    //hdr->type = htons(hdr->type);
    hdr->size = ntohs(hdr->size);
    hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
    hdr->timestamp_nsec = ntohl(hdr->timestamp_nsec);

    int16_t pl_length = ntohs(hdr->size);

    // check if the payload size is non-zero
    if (pl_length != 0) {
        // call to malloc for unknown payload length
        void *plp = calloc(pl_length, 1);
        *payloadp = plp;

        // save payload length to decrement for short counts
        //uint16_t pl_length = ntohs(hdr->size);
        //debug("pl_length: %d", pl_length);

        // call read
        /*int rval_for_pl = */read(fd, *payloadp, pl_length);
        //debug("read rval again for pl: %d", rval_for_pl);

        // check for short counts and re-execute read if there are short counts
        /*while (rval_for_pl) {
            if (rval_for_pl < 0) {
                errno = rval_for_pl;
                return -1;
            }
            pl_length = pl_length - rval_for_pl;
            rval_for_pl = Read(fd, *payloadp, pl_length);
        }*/
    }

    // no errors, exit successfully
    return 0;
}