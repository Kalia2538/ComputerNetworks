/*
 * transport.c 
 *
 * EN.601.414/614: HW#3 (STCP)
 *
 * This file implements the STCP layer that sits between the
 * mysocket and network layers. You are required to fill in the STCP
 * functionality in this file. 
 *
 */


#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "mysock.h"
#include "stcp_api.h"
#include "transport.h"
#include <arpa/inet.h>


#define WIN_SIZE 3072 //
#define STCPHdrSize sizeof(STCPHeader)




enum { 
    CSTATE_ESTABLISHED,
    CSTATE_CLOSED,
    CSTATE_LISTEN,
    CSTATE_SYN_SENT,
    CSTATE_SYN_RECEIVED,
    CSTATE_SYN_ACK_SENT,
    CSTATE_SYN_ACK_RECEIVED,
    CSTATE_FIN_SENT,
    CSTATE_FIN_RECIEVED,
    CSTATE_CLOSE_WAIT,
    CSTATE_LAST_ACK

};    /* obviously you should have more states */




/* this structure is global to a mysocket descriptor */
typedef struct
{
    bool_t done;    /* TRUE once connection is closed */

    int connection_state;   /* state of the connection (established, etc.) */
    tcp_seq initial_sequence_num;

    /* any other connection-wide global variables go here */
    tcp_seq seq;
    tcp_seq recv_seq;
    unsigned int recv_window_size;
    uint32_t recv_next_byte;
    uint32_t recv_prev_byte;
    tcp_seq peer_seq;
    tcp_seq win_begin;
    tcp_seq prev_ack;
    int recv_fin; // 0 if false 1 if true
    int is_active; // 0 if not active 1 if active
    int sent_ack; //0 if false, 1 if true
    int sent_fin;
    int fin_ack; // 0 for false, 1 for true


} context_t;

// struct to return both the event type and STCP Header in wait_forPacket()

typedef struct 
{
    int event;
    STCPHeader * hdr;
} event_hdr_t;



static void generate_initial_seq_num(context_t *ctx);
static void control_loop(mysocket_t sd, context_t *ctx);
static void event_close(mysocket_t sd, context_t * context);
static void event_ntwrk(mysocket_t sd, context_t * context);
static void event_app(mysocket_t sd, context_t * context);
static event_hdr_t* wait_for_packet(mysocket_t sd, context_t *context);
static ssize_t send_syn_ack(mysocket_t sd, int seq_num, unsigned int ack_num, uint8_t flag, uint16_t window);



/* initialise the transport layer, and start the main loop, handling
 * any data from the peer or the application.  this function should not
 * return until the connection is closed.
 */
void transport_init(mysocket_t sd, bool_t is_active)
{
    context_t *ctx;

    ctx = (context_t *) calloc(1, sizeof(context_t));
    assert(ctx);

    generate_initial_seq_num(ctx);

    /* XXX: you should send a SYN packet here if is_active, or wait for one
     * to arrive if !is_active.  after the handshake completes, unblock the
     * application with stcp_unblock_application(sd).  you may also use
     * this to communicate an error condition back to the application, e.g.
     * if connection fails; to do so, just set errno appropriately (e.g. to
     * ECONNREFUSED, etc.) before calling the function.
     */

    if (is_active) {
        // send syn packet
        ssize_t num = send_syn_ack(sd, ctx->initial_sequence_num, 0, TH_SYN, htons(WIN_SIZE));
        if (num < (ssize_t)0) {
            printf("[ERROR - ACTIVE OPEN]: error sending initial syn packet\n");
            return;
        }
        // updating the state
        ctx->connection_state = CSTATE_SYN_SENT;

        // wait for syn ack
        while(1) {
            event_hdr_t * e_hdr = wait_for_packet(sd, ctx);
            if (e_hdr->event == 2) { // we recieved network data
                STCPHeader * header = e_hdr->hdr;
                // check for SYN & ACK (ignore other flags) and seq nums

                // TODO: make this a helper function (synack_update_context)for readability
                if (((header->th_flags & (TH_SYN | TH_ACK)) == (TH_SYN | TH_ACK)) && (((tcp_seq)ntohl(header->th_ack)) == ctx->seq)) {
                    // weve received a syn-ack packet
                    // set rec_window_size
                    if (ntohs(header->th_win) > 0) {
                        ctx->recv_window_size = ntohs(header->th_win);
                    } else {
                        ctx->recv_window_size = 1;
                    }
                    ctx->recv_next_byte = ntohl(header->th_seq) + 1;
                    ctx->recv_prev_byte = ntohl(header->th_seq);
                    ctx->peer_seq = ntohl(header->th_seq);
                    ctx->win_begin = ctx->recv_next_byte;
                    // do stuff
                    break;
                }
            } 
        }
        ctx->connection_state = CSTATE_SYN_ACK_RECEIVED;

        // send ack
        num = send_syn_ack(sd, ctx->seq, ctx->recv_next_byte, TH_ACK, htons(WIN_SIZE));
        if (num < (ssize_t)0) {
            printf("[ERROR - ACTIVE OPEN]: error sending ack packet\n");
            return;
        }
        ctx->seq++;

    } else {
        // wait for syn
        while(1) {
            event_hdr_t * e_hdr = wait_for_packet(sd, ctx);
            if (e_hdr->event == 2) { // we recieved network data
                STCPHeader * header = e_hdr->hdr;
                // check for SYN & ACK (ignore other flags) and seq nums

                // TODO: make this a helper function (syn_update_context)for readability
                if ((header->th_flags & TH_SYN)) {
                    // weve received a synpacket
                    // set rec_window_size
                    if (ntohs(header->th_win) > 0) {
                        ctx->recv_window_size = ntohs(header->th_win);
                    } else {
                        ctx->recv_window_size = 1;
                    }
                    ctx->recv_next_byte = ntohl(header->th_seq) + 1;
                    ctx->recv_prev_byte = ntohl(header->th_seq);
                    ctx->peer_seq = ntohl(header->th_seq);
                    break;
                }
            } 
        }
        ctx->connection_state = CSTATE_SYN_RECEIVED;

        // send syn ack
        ssize_t num = send_syn_ack(sd, ctx->seq, ctx->recv_next_byte, (TH_SYN | TH_ACK), htons(WIN_SIZE));
        if (num < (ssize_t)0) {
            printf("[ERROR - ACTIVE OPEN]: error sending ack packet\n");
            return;
        }
        ctx->seq++;

        // wait for ack
        while(1) {
            event_hdr_t * e_hdr = wait_for_packet(sd, ctx);
            if (e_hdr->event == 2) { // we recieved network data
                STCPHeader * header = e_hdr->hdr;
                // check ACK (ignore other flags) and seq nums

                // TODO: make this a helper function (syn_update_context)for readability
                if ((header->th_flags & (TH_ACK)) && (((tcp_seq)ntohl(header->th_ack)) == ctx->seq)) {
                    // weve received a ack packet
                    // set rec_window_size
                    if (ntohs(header->th_win) > 0) {
                        ctx->recv_window_size = ntohs(header->th_win);
                    } else {
                        ctx->recv_window_size = 1;
                    }
                    ctx->recv_next_byte = ntohl(header->th_seq) + 1;
                    ctx->recv_prev_byte = ntohl(header->th_seq);
                    ctx->win_begin = ctx->recv_next_byte;
                    break;
                }
            } 
        }

    }
    ctx->connection_state = CSTATE_ESTABLISHED;
    stcp_unblock_application(sd);

    control_loop(sd, ctx);

    /* do any cleanup here */
    free(ctx);
}


/* generate random initial sequence number for an STCP connection */
static void generate_initial_seq_num(context_t *ctx)
{
    assert(ctx);

#ifdef FIXED_INITNUM
    /* please don't change this! */
    ctx->initial_sequence_num = 1;
#else
    /* you have to fill this up */
    /*ctx->initial_sequence_num =;*/
    srand(time(NULL)); // seed the rng based on time (to get different sequences)
    ctx->initial_sequence_num = rand() % 256; 
#endif
}


/* control_loop() is the main STCP loop; it repeatedly waits for one of the
 * following to happen:
 *   - incoming data from the peer
 *   - new data from the application (via mywrite())
 *   - the socket to be closed (via myclose())
 *   - a timeout
 */
static void control_loop(mysocket_t sd, context_t *ctx)
{
    assert(ctx);

    while (!ctx->done)
    {
        unsigned int event;

        /* see stcp_api.h or stcp_api.c for details of this function */
        /* XXX: you will need to change some of these arguments! */
        event = stcp_wait_for_event(sd, ANY_EVENT, NULL);

        /* check whether it was the network, app, or a close request */
        if (event & APP_DATA)
        {
            /* the application has requested that data be sent */
            /* see stcp_app_recv() */
            event_app(sd, ctx);
        }

        if (event & NETWORK_DATA) {
            /* received data from STCP peer */
            event_ntwrk(sd,ctx);
        }

        if (event & APP_CLOSE_REQUESTED) {
            event_close(sd, ctx);
        }

        /* etc. */
    }
}


/**********************************************************************/
/* our_dprintf
 *
 * Send a formatted message to stdout.
 * 
 * format               A printf-style format string.
 *
 * This function is equivalent to a printf, but may be
 * changed to log errors to a file if desired.
 *
 * Calls to this function are generated by the dprintf amd
 * dperror macros in transport.h
 */
void our_dprintf(const char *format,...)
{
    va_list argptr;
    char buffer[1024];

    assert(format);
    va_start(argptr, format);
    vsnprintf(buffer, sizeof(buffer), format, argptr);
    va_end(argptr);
    fputs(buffer, stdout);
    fflush(stdout);
}


// creates and sends a packet w/ given parameters
static ssize_t send_syn_ack(mysocket_t sd, int seq_num, unsigned int ack_num, uint8_t flag, uint16_t window){
    STCPHeader * packet = (STCPHeader *) malloc(STCPHdrSize);
    packet->th_seq = htonl(seq_num);
    packet->th_ack = htonl(ack_num);
    packet->th_off = 5;
    packet->th_flags = flag;
    packet->th_win = window;

    size_t val = stcp_network_send(sd, packet, STCPHdrSize);
    free(packet);
    return val;
}

// ret.event = -2 => malloc error
// ret.event = -1 => wrong hdr size
static event_hdr_t* wait_for_packet(mysocket_t sd, context_t *context) {
    char buff[sizeof(STCPHeader)]; // TODO: shoudl this be sctp_mss???
    event_hdr_t * ret = (event_hdr_t *)malloc(sizeof(event_hdr_t));
    ret->event = -2;
    return ret;

    // TODO: Error check this malloc call
    while (1) {
        // wait until we recieve a packet
        int event = stcp_wait_for_event(sd, NETWORK_DATA, NULL);
        // recieve data from the event
        ssize_t recv = stcp_network_recv(sd, buff, STCP_MSS);

        if (recv < (ssize_t)STCPHdrSize) {
            printf("[ERROR - RECV PKT]: recieved less than STCP Header Size\n");
            ret->event = -1;
            free(ret);
            return ret;
        }
        ret->event = event;
        ret->hdr = (STCPHeader *) buff; // TODO: Do i need to error check this first?
        return ret;
    }
}

static void event_app(mysocket_t sd, context_t * context) {
    tcp_seq in_flight_bytes = context->seq - context->prev_ack;
    unsigned int rem_win = WIN_SIZE - in_flight_bytes;
    if (in_flight_bytes > WIN_SIZE) {
        rem_win = 0;
    } else{
        rem_win = WIN_SIZE - in_flight_bytes;
    }

    if (rem_win == 0) {
        printf("[ERROR - EVENT_APP]: rem_win <= 0");
        return;
    }

    size_t to_send_size;
    if (rem_win < 0) {
        to_send_size = rem_win;
    } else {
        to_send_size = 100;
    }
    char buff[100];
    ssize_t rec_size = stcp_app_recv(sd, buff, to_send_size);
    // convert buff to stcp hdr
    STCPHeader * rec_hdr = (STCPHeader * ) buff;

    STCPHeader * ack_hdr = (STCPHeader *) malloc(STCPHdrSize);
    ack_hdr->th_seq = htonl(context->seq);
    ack_hdr->th_ack = htonl(context->recv_next_byte);
    ack_hdr->th_off = 5;
    ack_hdr->th_flags = TH_ACK;
    ack_hdr->th_win = htons(WIN_SIZE - (context->recv_next_byte - context->recv_prev_byte));

    char * msg = (char *) malloc(STCPHdrSize + rec_size);
    memcpy(msg, ack_hdr, STCPHdrSize);
    memcpy(msg + STCPHdrSize, rec_hdr, rec_size);
    stcp_network_send(sd, msg, STCPHdrSize + rec_size);
    context->seq += rec_size;

    free(ack_hdr);
    free(msg);

    return;

}

static void event_ntwrk(mysocket_t sd, context_t * context) {
    size_t max_size = STCP_MSS + STCPHdrSize;
    char buff[max_size];
    ssize_t rec_size = stcp_network_recv(sd, buff, max_size);
    size_t rec_msg_len = rec_size - STCPHdrSize;    
    STCPHeader * rec_hdr = (STCPHeader *) buff;
    context->recv_window_size = rec_hdr->th_win;


    uint16_t req_seq = ntohl(rec_hdr->th_seq);
    uint8_t rec_flags = rec_hdr->th_flags;


    // if we get a duplicate packet
    if (context->recv_seq > req_seq) {
        ssize_t num = send_syn_ack(sd, context->seq, context->recv_next_byte, TH_ACK, htons(WIN_SIZE));
        if (num < (ssize_t)0) {
            printf("[ERROR - NETWORK EVENT]: error sending ack packet\n");
            return;
        }
    }

    // if we have an ack
    if (rec_flags & TH_ACK) {
        uint32_t rec_ack = ntohl(rec_hdr->th_ack);

        // update prev ack
        if (rec_ack > context->prev_ack) {
            context->prev_ack = rec_ack;
        }
    }
    // ----
    if (rec_size > STCPHdrSize ) {
        // check for fin flag
        uint16_t seq_n_size = req_seq + rec_msg_len;
        if (rec_flags & TH_FIN) {
            context->recv_prev_byte = seq_n_size;
            context->recv_next_byte = seq_n_size + 1;
            context->recv_fin = 1;
        } else { // no fin flag
            context->recv_prev_byte = seq_n_size - 1;
            context->recv_next_byte = seq_n_size;
        }

        // send an ack
        ssize_t num = send_syn_ack(sd, context->seq, context->recv_next_byte, TH_ACK, htons(WIN_SIZE - (context->recv_next_byte - context->recv_prev_byte)));
        if (num < (ssize_t)0) {
            printf("[ERROR - NETWORK EVENT]: error sending ack packet\n");
            return;
        }

        char * msg = (char *) malloc(rec_msg_len);
        memcpy(msg, buff + STCPHdrSize, rec_msg_len);
        stcp_app_send(sd, msg, rec_msg_len);
        free(msg);

        if (context->is_active == 0) {
            stcp_fin_received(sd);
        }

    } else { // ----
        if ((context->recv_fin == 0) && (rec_flags & TH_FIN)) {
            context->recv_fin = 1;
            context->prev_ack = req_seq;
            context->recv_next_byte = req_seq + 1;

            if (context->seq < WIN_SIZE + context->win_begin) {
                ssize_t num = send_syn_ack(sd, context->seq, context->recv_next_byte, TH_ACK, htons(WIN_SIZE - (context->recv_next_byte - context->recv_prev_byte)));
                if (num < (ssize_t)0) {
                    printf("[ERROR - NETWORK EVENT]: error sending ack packet\n");
                    return;
                }
                context->sent_ack = 1;
            }

            // check if we need to send a fin
            if (context->sent_fin == 0) {
                ssize_t num = send_syn_ack(sd, context->seq, context->recv_next_byte, (TH_ACK | TH_FIN), htons(WIN_SIZE - (context->recv_next_byte - context->recv_prev_byte)));
                if (num < (ssize_t)0) {
                    printf("[ERROR - NETWORK EVENT]: error sending fin packet\n");
                    return;
                }
                context->sent_fin = 1;
                context->seq++;
            }
            // fin sent
            // no fin ack
            // hdrflags & ack
            //ack = seq
            // todo: make this look shorter ... possibly pre-cal vars above
        } else if ((rec_hdr->th_flags & TH_ACK) && context->sent_fin && context->fin_ack && ((ntohl(rec_hdr->th_ack)) == context->seq)){
            context->fin_ack = 1;
            
            context->recv_prev_byte = ntohl(rec_hdr->th_seq);
            context->recv_next_byte = ntohl(rec_hdr->th_seq) + 1;

            context->win_begin = context->recv_next_byte;
        } else {
            context->win_begin = context->recv_next_byte;
        }
    }


    // finisheddddddddd
    if (context->sent_fin && context->recv_fin && context->fin_ack) {
        context->done = TRUE; 
    }

}

static void event_close(mysocket_t sd, context_t * context) {
    if((context->is_active == 1) && (context->sent_fin == 0)) {
        ssize_t num = send_syn_ack(sd, context->seq, context->recv_next_byte, (TH_ACK | TH_FIN), htons(WIN_SIZE - (context->recv_next_byte - context->recv_prev_byte)));
        if (num < (ssize_t)0) {
            printf("[ERROR - CLOSE EVENT]: error sending fin packet\n");
            return;
        }
        context->sent_fin = 1;
        context->seq ++;
    }
}
