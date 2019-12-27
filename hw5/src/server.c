#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "csapp.h"
#include "server.h"
#include "protocol.h"

// handle ALL client requests through this function
void *brs_client_service(void *arg) {
    // log in flag, trader declared
    int logged = 0;
    TRADER *first_trader = NULL;
    BRS_STATUS_INFO *status_info = calloc(sizeof(BRS_STATUS_INFO), 1);

    // save the file descriptor
    int connfdp = *((int *) arg);

    // detach thread
    Pthread_detach(Pthread_self());

    // free up argument
    free(arg);

    // register the client with the file descriptor
    creg_register(client_registry, connfdp);

    // service loop to accept client requests
    while (1) {
        // check if logged
        if (logged == 0) {
            // declare variables for storage
            int user_length;
            int type;
            char *user;
            int skip_bytes;

            // read two bytes for type (2 for uint8_t because of padding)
            int read_rval = read(connfdp, &type, 2);

            // no "type" in packet
            if (read_rval <= 0) {
                // ignore packet
                continue;
            }

            // packet type PROCESSING for ZERO logins
            if (type == BRS_LOGIN_PKT) {
                // read bytes for the name length
                read_rval = read(connfdp, &user_length, 2);

                // no "size" in packet
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // convert user length to host order
                user_length = ntohs(user_length);

                // skip 8 bytes to get to payload (should be non-null)
                read_rval = read(connfdp, &skip_bytes, 8);

                // fail skip
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // now we get to the payload area: calloc space for user
                user = (char *) calloc(sizeof(char), user_length);

                // read bytes for the username
                read_rval = read(connfdp, user, user_length);

                // reading bytes into user error
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // login the user
                first_trader = trader_login(connfdp, user);

                // send ACK if non-null, send NACK if null
                if (first_trader != NULL) {
                    exchange_get_status(exchange, status_info);
                    trader_send_ack(first_trader, NULL);
                } else {
                    trader_send_nack(first_trader);
                }

                // upon complete, increment log flag
                logged = 1;

            } else {
                // if it's NOT a login packet, we have to continue until it is one
                continue;
            }

        } else if (logged == 1) {
            // we are logged in, so now we can process other packets EXCEPT login
            // variables for storage
            int type;

            // read two bytes for type (2 for uint8_t because of padding)
            int read_rval = read(connfdp, &type, 2);

            // no "type" in packet
            if (read_rval == 0) {
                // ignore packet
                continue;
            }

            // check types for processing
            if (type == BRS_LOGIN_PKT) { //---------------------------------LOGIN x2
                // continue because we are already logged on
                continue;

            } else if (type == BRS_STATUS_PKT) { // ------------------------STATUS
                // send the ACK packet w/ status info
                exchange_get_status(exchange, status_info);
                trader_send_ack(first_trader, status_info);

            } else if (type == BRS_DEPOSIT_PKT) { // -----------------------DEPOSIT
                // declare vars for storage (may need mutex??)
                int payload_size;
                int skip_bytes;
                funds_t funds;

                // grab payload size
                read_rval = read(connfdp, &payload_size, 2);

                // convert payload size to host order
                payload_size = ntohs(payload_size);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // skip to payload
                read_rval = read(connfdp, &skip_bytes, 8);

                // fail skip
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // currently at packet, take the funds
                read_rval = read(connfdp, &funds, payload_size);

                // fail read
                if (read_rval <= 0) {
                     // ignore packet
                    continue;
                }

                // convert funds to host order
                funds = ntohl(funds);

                // add funds to balance
                trader_increase_balance(first_trader, funds);

                // send an ACK packet
                exchange_get_status(exchange, status_info);
                trader_send_ack(first_trader, status_info);

            } else if (type == BRS_WITHDRAW_PKT) { // ------------------ WITHDRAW
                // declare vars for storage (may need mutex??)
                int payload_size;
                int skip_bytes;
                funds_t funds;

                // grab payload size
                read_rval = read(connfdp, &payload_size, 2);

                // convert payload size to host order
                payload_size = ntohs(payload_size);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // skip to payload
                read_rval = read(connfdp, &skip_bytes, 8);

                // fail skip
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // currently at packet, take the funds
                read_rval = read(connfdp, &funds, payload_size);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // convert funds to host order
                funds = ntohl(funds);

                // add funds to balance
                int balance_check = trader_decrease_balance(first_trader, funds);

                // fail to decrease
                if (balance_check == -1) {
                    // make no changes and send NACK
                    trader_send_nack(first_trader);
                    // next request
                    continue;
                }

                // send an ACK packet
                exchange_get_status(exchange, status_info);
                trader_send_ack(first_trader, status_info);

            } else if (type == BRS_ESCROW_PKT) { // ---------------------- ESCROW
                // declare vars for storage (may need mutex??)
                int payload_size;
                int skip_bytes;
                quantity_t escrow_amount;

                // grab payload size
                read_rval = read(connfdp, &payload_size, 2);

                // convert payload size to host order
                payload_size = ntohs(payload_size);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // skip to payload
                read_rval = read(connfdp, &skip_bytes, 8);

                // fail skip
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // currently at packet, take the stock
                read_rval = read(connfdp, &escrow_amount, payload_size);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // convert stock to host order
                escrow_amount = ntohl(escrow_amount);

                // add stock to inventory
                trader_increase_inventory(first_trader, escrow_amount);

                // send an ACK packet
                exchange_get_status(exchange, status_info);
                trader_send_ack(first_trader, status_info);

            } else if (type == BRS_RELEASE_PKT) { // --------------------- RELEASE
                // declare vars for storage (may need mutex??)
                int payload_size;
                int skip_bytes;
                quantity_t escrow_amount;

                // grab payload size
                read_rval = read(connfdp, &payload_size, 2);

                // convert payload size to host order
                payload_size = ntohs(payload_size);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // skip to payload
                read_rval = read(connfdp, &skip_bytes, 8);

                // fail skip
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // currently at packet, take the stock
                read_rval = read(connfdp, &escrow_amount, payload_size);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // convert stock to host order
                escrow_amount = ntohl(escrow_amount);

                // add stock to balance
                int inventory_check = trader_decrease_inventory(first_trader, escrow_amount);

                // fail to decrease
                if (inventory_check == -1) {
                    // make no changes and send NACK
                    trader_send_nack(first_trader);
                    // next request
                    continue;
                }

                // send an ACK packet
                exchange_get_status(exchange, status_info);
                trader_send_ack(first_trader, status_info);

            } else if (type == BRS_BUY_PKT) { // ------------------------- BUY
                // declare vars for storage (may need mutex??)
                int payload_size;
                int skip_bytes;
                quantity_t stock_amount;
                funds_t price_per;

                // grab payload size
                read_rval = read(connfdp, &payload_size, 2);

                // convert payload size to host order
                payload_size = ntohs(payload_size);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // skip to payload
                read_rval = read(connfdp, &skip_bytes, 8);

                // fail skip
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // currently at packet, take the stock
                read_rval = read(connfdp, &stock_amount, payload_size / 2);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // convert funds to host order
                stock_amount = ntohl(stock_amount);

                // currently at packet, take the price
                read_rval = read(connfdp, &price_per, payload_size / 2);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // convert funds to host order
                price_per = ntohl(price_per);

                // post the buy/sell and get order ID
                orderid_t order_id = exchange_post_buy(exchange, first_trader, stock_amount, price_per);

                // error checks
                if (order_id == 0) {
                    // make no changes and send NACK packet
                    trader_send_nack(first_trader);
                    // next request
                    continue;
                }

                // convert order_id to host order
                order_id = ntohl(order_id);

                // update the struct
                status_info->orderid = order_id;

                // send ACK and set order id in status struct
                exchange_get_status(exchange, status_info);
                trader_send_ack(first_trader, status_info);

            } else if (type == BRS_SELL_PKT) { // ------------------------ SELL
                // declare vars for storage (may need mutex??)
                int payload_size;
                int skip_bytes;
                quantity_t stock_amount;
                funds_t price_per;

                // grab payload size
                read_rval = read(connfdp, &payload_size, 2);

                // convert payload size to host order
                payload_size = ntohs(payload_size);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // skip to payload
                read_rval = read(connfdp, &skip_bytes, 8);

                // fail skip
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // currently at packet, take the stock
                read_rval = read(connfdp, &stock_amount, payload_size / 2);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // convert funds to host order
                stock_amount = ntohl(stock_amount);

                // currently at packet, take the price
                read_rval = read(connfdp, &price_per, payload_size / 2);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // convert funds to host order
                price_per = ntohl(price_per);

                // post the buy/sell and get order ID
                orderid_t order_id = exchange_post_sell(exchange, first_trader, stock_amount, price_per);

                // error checks
                if (order_id == 0) {
                    // make no changes and send NACK packet
                    trader_send_nack(first_trader);
                    // next request
                    continue;
                }

                // convert order_id to host order
                order_id = ntohl(order_id);

                // update the struct
                status_info->orderid = order_id;

                // send ACK and set order id in status struct
                exchange_get_status(exchange, status_info);
                trader_send_ack(first_trader, status_info);

            } else if (type == BRS_CANCEL_PKT) { // ---------------------- CANCEL
                // declare vars for storage (may need mutex??)
                int payload_size;
                int skip_bytes;
                orderid_t order_id;

                // grab payload size
                read_rval = read(connfdp, &payload_size, 2);

                // convert payload size to host order
                payload_size = ntohs(payload_size);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // skip to payload
                read_rval = read(connfdp, &skip_bytes, 8);

                // fail skip
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // currently at packet, take the stock
                read_rval = read(connfdp, &order_id, payload_size);

                // fail read
                if (read_rval <= 0) {
                    // ignore packet
                    continue;
                }

                // convert funds to host order
                order_id = ntohl(order_id);

                // cancel the order
                int cancel_check = exchange_cancel(exchange, first_trader, order_id, &status_info->quantity);

                // cancel order fail
                if (cancel_check == -1) {
                    // NACK packet
                    trader_send_nack(first_trader);
                    // next request
                    continue;
                }

                // send ACK packet with updates
                exchange_get_status(exchange, status_info);
                trader_send_ack(first_trader, status_info);

            } else {
                // invalid packet type, continue (ignore invalid packets)
                continue;
            }

        }
        // END OF WHILE LOOP, go back to beginning of loop
    }

    // close file descriptor
    close(connfdp);
    return NULL;
}