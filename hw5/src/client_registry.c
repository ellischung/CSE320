#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <semaphore.h>

#include "csapp.h"
#include "debug.h"
#include "server.h"
#include "exchange.h"
#include "protocol.h"
#include "client_registry.h"

// flag for termination
volatile int term_flag = 0;

// client struct
struct client_registry {
    int *connections;
    sem_t mutex;
    sem_t lock;
};

// initialize the client struct
CLIENT_REGISTRY *creg_init() {
    // allocate memory for the client struct
    CLIENT_REGISTRY *cr = (CLIENT_REGISTRY *) calloc(sizeof(struct client_registry), 1);

    // allocate memory for connections (clients)
    cr->connections = calloc(sizeof(int), FD_SETSIZE);

    // initialize semaphores
    sem_init(&(cr->mutex), 0, 1);
    sem_init(&(cr->lock), 0, 0);

    // return the struct
    return cr;
}

// finialize a client registry
void creg_fini(CLIENT_REGISTRY *cr) {
    // set termination flag on
    term_flag = 1;
    // if cr is not null, use sem wait and post and also free cr
    if (cr != NULL) {
        sem_wait(&(cr->mutex));
        free(cr->connections);
        free(cr);
        sem_post(&(cr->mutex));
        cr = NULL;
    }
}

// register a client
int creg_register(CLIENT_REGISTRY *cr, int fd) {
    // use sem_wait and sem_post for protection
    sem_wait(&(cr->mutex));

    // if there's a non-null client, register the connection
    if (cr != NULL) {
        int i;
        for (i = 0; i < FD_SETSIZE; i++) {
            if (*(cr->connections + i) != 0) {
                i++;
                continue;
            }
            break;
        }
        *(cr->connections + i) = fd;
    } else {
        // null client registry, return -1
        sem_post(&(cr->mutex));
        return -1;
    }

    // successful register, return 0
    sem_post(&(cr->mutex));
    return 0;
}

// unregister a client
int creg_unregister(CLIENT_REGISTRY *cr, int fd) {
    // use sem_wait and sem_post for protection
    sem_wait(&(cr->mutex));

    // if there's a non-null client, deregister the connection
    if (cr != NULL) {
        int i;
        for (i = 0; i < FD_SETSIZE; i++) {
            if (*(cr->connections + i) != fd) {
                i++;
                continue;
            }
            *(cr->connections + i) = 0;
            break;
        }
    } else {
        // null client registry, return -1
        sem_post(&(cr->mutex));
        return -1;
    }

    // successful deregister, return 0
    sem_post(&(cr->mutex));
    return 0;
}

// wait for empty
void creg_wait_for_empty(CLIENT_REGISTRY *cr) {
    // if flag for termination has not been set, use sem wait and return
    if (!term_flag) {
        sem_wait(&(cr->lock));
    }
    return;
}

// shutdown with SHUT_RD on all registered fd's
void creg_shutdown_all(CLIENT_REGISTRY *cr) {
    // shutdown ALL fd's, surround with semaphores
    sem_wait(&(cr->mutex));
    int i;
    for (i = 0; i < FD_SETSIZE; i++) {
        if (*(cr->connections + i) != 0) {
            *(cr->connections + i) = 0;
            shutdown(*(cr->connections + i), SHUT_RD);
        }
        else {
            shutdown(*(cr->connections + i), SHUT_RD);
        }
    }
    sem_post(&(cr->mutex));
    return;
}