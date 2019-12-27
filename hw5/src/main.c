#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "csapp.h"
#include "client_registry.h"
#include "exchange.h"
#include "protocol.h"
#include "trader.h"
#include "debug.h"
#include "server.h"

extern EXCHANGE *exchange;
extern CLIENT_REGISTRY *client_registry;

static void terminate(int status);

void sighup_handler(int sig);

/*
 * "Bourse" exchange server.
 *
 * Usage: bourse <port>
 */
int main(int argc, char* argv[]){
    // set up signal handler for SIGHUP
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sighup_handler;
    sa.sa_flags = 0;
    sigaction(SIGHUP, &sa, NULL);

    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    if (argc != 3) {
        fprintf(stderr, "Usage: bin/bourse -p <port>\n");
        exit(0);
    }
    if (strcmp(argv[1], "-p")) {
        fprintf(stderr, "Usage: bin/bourse -p <port>\n");
        exit(0);
    }

    // Perform required initializations of the client_registry,
    // maze, and player modules.
    client_registry = creg_init();
    exchange = exchange_init();
    trader_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function brs_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    // csapp function call
    listenfd = Open_listenfd(argv[2]);

    while(1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Pthread_create(&tid, NULL, brs_client_service, connfdp);
    }

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    exchange_fini(exchange);
    trader_fini();

    debug("Bourse server terminating");
    exit(status);
}

void sighup_handler(int sig) {
    terminate(EXIT_SUCCESS);
}
