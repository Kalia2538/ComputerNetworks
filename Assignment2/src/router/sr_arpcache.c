#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include "sr_arpcache.h"
#include "sr_router.h"
#include "sr_if.h"
#include "sr_protocol.h"

/*
  This function gets called every second. For each request sent out, we keep
  checking whether we should resend an request or destroy the arp request.
  See the comments in the header file for an idea of what it should look like.
*/
void sr_arpcache_sweepreqs(struct sr_instance *sr) {
    /* fill in code here */
    struct sr_arpreq *curr = sr->cache.requests;
    while (curr != NULL) {
        handle_arpreq(sr, curr);
        if (curr->times_sent >= 5) {
            sr_arpreq_destroy(&sr->cache, curr);
        }
        curr = curr->next;
    }
}

void handle_arpreq(struct sr_instance *sr, struct sr_arpreq *request) {
    /* fill in code here */
    // gets the current time
    time_t now;
    time(&now);
    // check if it has been more than a second since this request was last sent
    if (difftime(now, request->sent) > 1.0) {
        // check # of times req was sent b4
        if (request->times_sent >= 5) {
            // we've sent this request the max # of times -> need to send ICMP messages
            /* TODO: find all packets waiting on this request and then send the ICMP message to the 
                src address in each of those packets*/
                struct sr_packet* curr = request->packets;
                while(curr != NULL) {
                    uint8_t * packet = (uint8_t *) curr->buf;
                    sr_ethernet_hdr_t * pkt_ethhdr =  (sr_ethernet_hdr_t*) packet;
                    sr_ip_hdr_t * pkt_iphdr = (sr_ip_hdr_t *) (packet + sizeof(sr_ethernet_hdr_t));
                    sr_icmp_hdr_t * pkt_icmphdr = (sr_icmp_hdr_t*) (packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));

                    uint8_t * pkteth = pkt_ethhdr->ether_dhost;
                    struct sr_if * iface = get_interface_from_eth(sr, pkteth);
                    if (iface == NULL) {
                        curr = curr->next;
                        continue;
                    }
                    // send an icmp message

                    uint8_t * icmp_msg = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
                    if (icmp_msg == NULL) {
                    printf("malloc failed\n");
                    return;
                    }            
                    
                    unsigned int packet_length = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
                    // initialize to 0
                    memset(icmp_msg, 0, packet_length);

                    
        
                    // set the icmp hdr
                    sr_icmp_hdr_t *new_hdr_icmp = (sr_icmp_hdr_t *)(icmp_msg + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
                    
                    new_hdr_icmp->icmp_type = 3;
                    new_hdr_icmp->icmp_sum = 1;
                    // update icmp data
                    memcpy(new_hdr_icmp->data, pkt_iphdr, sizeof(sr_ip_hdr_t) + 8 );
        
        
                    uint16_t i_sum = cksum(new_hdr_icmp, packet_length - sizeof(sr_ip_hdr_t) - sizeof(sr_ethernet_hdr_t));
                    new_hdr_icmp->icmp_sum = i_sum;
        
                    // set the ip header
                    sr_ip_hdr_t *new_ip_hdr = (sr_ip_hdr_t *) (icmp_msg + sizeof(sr_ethernet_hdr_t));
                    new_ip_hdr->ip_tos = pkt_iphdr->ip_tos; // do i need to use memcpy here?
                    // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
                    new_ip_hdr->ip_len = htons(sizeof(packet - sizeof(sr_ethernet_hdr_t)));
                    new_ip_hdr->ip_ttl = INIT_TTL;
                    new_ip_hdr->ip_p = ip_protocol_icmp;
                    new_ip_hdr->ip_sum = 0;
                    new_ip_hdr->ip_src = iface->ip; // DC: is this okay
                    new_ip_hdr->ip_dst = pkt_iphdr->ip_src;
                    uint16_t ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));
                    new_ip_hdr->ip_sum = ip_sum;
                    // do i need to update new_ip_hdr->id?
                    
                    // set the ethernet header
                    struct sr_if * our_iface = sr_get_interface(sr, iface);
                    sr_ethernet_hdr_t *new_eth_hdr = (sr_ethernet_hdr_t*) (icmp_msg);
                    memcpy(new_eth_hdr->ether_dhost, pkt_ethhdr->ether_shost, ETHER_ADDR_LEN);
                    memcpy(new_eth_hdr->ether_shost, our_iface->addr, ETHER_ADDR_LEN);
                    new_eth_hdr->ether_type = htons(ethertype_ip);
                    
                    printf("icmp msg\n");
                    print_hdrs(icmp_msg, packet_length);
        
                    
                    // send the packet
                    
                    sr_send_packet(sr, icmp_msg, packet_length, iface);

                }

                
















            // struct sr_packet* curr = request->packets;
            // while (curr != NULL) {
            //     uint8_t * icmp_msg = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
            //     unsigned int packet_length = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);

            //     sr_ethernet_hdr_t * cur_hdr = (sr_ethernet_hdr_t *) curr;
            //     // set the ethernet header
            //     sr_ethernet_hdr_t *new_eth_hdr;
            //     struct sr_if * interface = find_rt_dest(sr, request->ip);
            //     memcpy(new_eth_hdr->ether_dhost, cur_hdr->ether_shost, ETHER_ADDR_LEN);
            //     memcpy(new_eth_hdr->ether_shost, interface->addr, ETHER_ADDR_LEN);
            //     new_eth_hdr->ether_type = htons(ethertype_ip);
    

            //     // set the ip header
            //     sr_ip_hdr_t *new_ip_hdr;
            //     struct sr_if * curr_interface = sr_get_interface(sr, curr->iface);
            //     new_ip_hdr->ip_tos = 0x0; // do i need to use memcpy here?
            //     // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
            //     new_ip_hdr->ip_len = htons(sizeof(cur_hdr - sizeof(sr_ethernet_hdr_t)));
            //     new_ip_hdr->ip_ttl = INIT_TTL;
            //     new_ip_hdr->ip_p = ip_protocol_icmp;
            //     new_ip_hdr->ip_sum = 0;
            //     new_ip_hdr->ip_src = interface->ip; // DC: is this okay
            //     new_ip_hdr->ip_dst = curr_interface->ip;
            //     uint16_t ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));
            //     new_ip_hdr->ip_sum = ip_sum;

                

            //     // set the icmp hdr
            //     sr_icmp_hdr_t *hdr_icmp;
            //     hdr_icmp->icmp_type = 3;
            //     hdr_icmp->icmp_code = 1;
            //     memcpy(hdr_icmp->data, curr->buf, sizeof(curr->buf)); // DC: make sure the 2nd and 3rd params are correct
            //     hdr_icmp->icmp_sum = 0;
            //     uint16_t icmp_sum = cksum(hdr_icmp, sizeof(sr_icmp_hdr_t));
            //     hdr_icmp->icmp_sum = icmp_sum;

                
            //     // construct the packet
            //     memcpy(icmp_msg, new_eth_hdr, sizeof(sr_ethernet_hdr_t));
            //     memcpy(icmp_msg + sizeof(sr_ethernet_hdr_t), new_ip_hdr, sizeof(sr_ip_hdr_t));
            //     memcpy(icmp_msg + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t), hdr_icmp, sizeof(sr_icmp_hdr_t));

            //     // find dest interface
            //     // send the packet
            //     printf("icmp msg\n");
            //     print_hdrs(icmp_msg, packet_length);

            //     sr_send_packet(sr, icmp_msg, packet_length, curr_interface->name);

            //     curr = curr->next;
            // }


        } else {
            if (request->sent == 0) {
                request->sent = now;
                request->times_sent = 1;
            } else {
                request->sent = now;
                request->times_sent++;
            }
            
            // allocate
            uint8_t * new_request = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t));
            memset(new_request, 0, sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t));
            // instantiate ethernet header
            struct sr_if * interface = find_rt_dest(sr, request->ip);
            // sr_get_interface(sr, entry->name);
            
            sr_ethernet_hdr_t *new_eth_hdr = (sr_ethernet_hdr_t *) new_request;
            memcpy(new_eth_hdr->ether_dhost, 0xFF, ETHER_ADDR_LEN);
            memcpy(new_eth_hdr->ether_shost, interface->addr, ETHER_ADDR_LEN);
            new_eth_hdr->ether_type = htons(ethertype_arp);

            // copy over the ethernet header
            // memcpy(new_request, new_eth_hdr, sizeof(sr_ethernet_hdr_t)); // TODO: double check the math here
            
            // instantiate arp header
            sr_arp_hdr_t * new_arp_hdr = (sr_arp_hdr_t*)(new_request + sizeof(sr_ethernet_hdr_t));
            new_arp_hdr->ar_hrd = htons(arp_hrd_ethernet);
            new_arp_hdr->ar_pro = htons(ethertype_arp);
            new_arp_hdr->ar_hln = ETHER_ADDR_LEN; // assuming this variable should be the same as last time
            new_arp_hdr->ar_pln = 0x04;
            new_arp_hdr->ar_op = htons(arp_op_request);
            memcpy(new_arp_hdr->ar_sha, interface->addr, ETHER_ADDR_LEN);
            new_arp_hdr->ar_sip = interface->ip;
            memcpy(new_arp_hdr->ar_tha, 0xFF, ETHER_ADDR_LEN);
            new_arp_hdr->ar_tip = request->ip; // DC THIS

            // copy over the new arp header
            // memcpy(new_request + sizeof(sr_ethernet_hdr_t), new_arp_hdr, sizeof(sr_arp_hdr_t));
            unsigned int length = sizeof(sr_ethernet_hdr_t)+sizeof(sr_arp_hdr_t);
            
            printf("packet(new_request) to be sent \n");
            print_hdrs(new_request, length);
            // send the packet
            sr_send_packet(sr, new_request, length, interface->name);
            free(new_request);
        }
    }
}

/* You should not need to touch the rest of this code. */

/* Checks if an IP->MAC mapping is in the cache. IP is in network byte order.
   You must free the returned structure if it is not NULL. */
struct sr_arpentry *sr_arpcache_lookup(struct sr_arpcache *cache, uint32_t ip) {
    pthread_mutex_lock(&(cache->lock));

    struct sr_arpentry *entry = NULL, *copy = NULL;

    int i;
    for (i = 0; i < SR_ARPCACHE_SZ; i++) {
        if ((cache->entries[i].valid) && (cache->entries[i].ip == ip)) {
            entry = &(cache->entries[i]);
        }
    }

    /* Must return a copy b/c another thread could jump in and modify
       table after we return. */
    if (entry) {
        copy = (struct sr_arpentry *) malloc(sizeof(struct sr_arpentry));
        memcpy(copy, entry, sizeof(struct sr_arpentry));
    }

    pthread_mutex_unlock(&(cache->lock));

    return copy;
}

/* Adds an ARP request to the ARP request queue. If the request is already on
   the queue, adds the packet to the linked list of packets for this sr_arpreq
   that corresponds to this ARP request. You should free the passed *packet.

   A pointer to the ARP request is returned; it should not be freed. The caller
   can remove the ARP request from the queue by calling sr_arpreq_destroy. */
struct sr_arpreq *sr_arpcache_queuereq(struct sr_arpcache *cache,
                                       uint32_t ip,
                                       uint8_t *packet,           /* borrowed */
                                       unsigned int packet_len,
                                       char *iface)
{
    pthread_mutex_lock(&(cache->lock));

    struct sr_arpreq *req;
    for (req = cache->requests; req != NULL; req = req->next) {
        if (req->ip == ip) {
            break;
        }
    }

    /* If the IP wasn't found, add it */
    if (!req) {
        req = (struct sr_arpreq *) calloc(1, sizeof(struct sr_arpreq));
        req->ip = ip;
        req->next = cache->requests;
        cache->requests = req;
    }

    /* Add the packet to the list of packets for this request */
    if (packet && packet_len && iface) {
        struct sr_packet *new_pkt = (struct sr_packet *)malloc(sizeof(struct sr_packet));

        new_pkt->buf = (uint8_t *)malloc(packet_len);
        memcpy(new_pkt->buf, packet, packet_len);
        new_pkt->len = packet_len;
		new_pkt->iface = (char *)malloc(sr_IFACE_NAMELEN);
        strncpy(new_pkt->iface, iface, sr_IFACE_NAMELEN);
        new_pkt->next = req->packets;
        req->packets = new_pkt;
    }

    pthread_mutex_unlock(&(cache->lock));

    return req;
}

/* This method performs two functions:
   1) Looks up this IP in the request queue. If it is found, returns a pointer
      to the sr_arpreq with this IP. Otherwise, returns NULL.
   2) Inserts this IP to MAC mapping in the cache, and marks it valid. */
struct sr_arpreq *sr_arpcache_insert(struct sr_arpcache *cache,
                                     unsigned char *mac,
                                     uint32_t ip)
{
    pthread_mutex_lock(&(cache->lock));

    struct sr_arpreq *req, *prev = NULL, *next = NULL;
    for (req = cache->requests; req != NULL; req = req->next) {
        if (req->ip == ip) {
            if (prev) {
                next = req->next;
                prev->next = next;
            }
            else {
                next = req->next;
                cache->requests = next;
            }

            break;
        }
        prev = req;
    }

    int i;
    for (i = 0; i < SR_ARPCACHE_SZ; i++) {
        if (!(cache->entries[i].valid))
            break;
    }

    if (i != SR_ARPCACHE_SZ) {
        memcpy(cache->entries[i].mac, mac, 6);
        cache->entries[i].ip = ip;
        cache->entries[i].added = time(NULL);
        cache->entries[i].valid = 1;
    }

    pthread_mutex_unlock(&(cache->lock));

    return req;
}

/* Frees all memory associated with this arp request entry. If this arp request
   entry is on the arp request queue, it is removed from the queue. */
void sr_arpreq_destroy(struct sr_arpcache *cache, struct sr_arpreq *entry) {
    pthread_mutex_lock(&(cache->lock));

    if (entry) {
        struct sr_arpreq *req, *prev = NULL, *next = NULL;
        for (req = cache->requests; req != NULL; req = req->next) {
            if (req == entry) {
                if (prev) {
                    next = req->next;
                    prev->next = next;
                }
                else {
                    next = req->next;
                    cache->requests = next;
                }

                break;
            }
            prev = req;
        }

        struct sr_packet *pkt, *nxt;

        for (pkt = entry->packets; pkt; pkt = nxt) {
            nxt = pkt->next;
            if (pkt->buf)
                free(pkt->buf);
            if (pkt->iface)
                free(pkt->iface);
            free(pkt);
        }

        free(entry);
    }

    pthread_mutex_unlock(&(cache->lock));
}

/* Prints out the ARP table. */
void sr_arpcache_dump(struct sr_arpcache *cache) {
    fprintf(stderr, "\nMAC            IP         ADDED                      VALID\n");
    fprintf(stderr, "-----------------------------------------------------------\n");

    int i;
    for (i = 0; i < SR_ARPCACHE_SZ; i++) {
        struct sr_arpentry *cur = &(cache->entries[i]);
        unsigned char *mac = cur->mac;
        fprintf(stderr, "%.1x%.1x%.1x%.1x%.1x%.1x   %.8x   %.24s   %d\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ntohl(cur->ip), ctime(&(cur->added)), cur->valid);
    }

    fprintf(stderr, "\n");
}

/* Initialize table + table lock. Returns 0 on success. */
int sr_arpcache_init(struct sr_arpcache *cache) {
    /* Seed RNG to kick out a random entry if all entries full. */
    srand(time(NULL));

    /* Invalidate all entries */
    memset(cache->entries, 0, sizeof(cache->entries));
    cache->requests = NULL;

    /* Acquire mutex lock */
    pthread_mutexattr_init(&(cache->attr));
    pthread_mutexattr_settype(&(cache->attr), PTHREAD_MUTEX_RECURSIVE);
    int success = pthread_mutex_init(&(cache->lock), &(cache->attr));

    return success;
}

/* Destroys table + table lock. Returns 0 on success. */
int sr_arpcache_destroy(struct sr_arpcache *cache) {
    return pthread_mutex_destroy(&(cache->lock)) && pthread_mutexattr_destroy(&(cache->attr));
}

/* Thread which sweeps through the cache and invalidates entries that were added
   more than SR_ARPCACHE_TO seconds ago. */
void *sr_arpcache_timeout(void *sr_ptr) {
    struct sr_instance *sr = sr_ptr;
    struct sr_arpcache *cache = &(sr->cache);

    while (1) {
        sleep(1.0);

        pthread_mutex_lock(&(cache->lock));

        time_t curtime = time(NULL);

        int i;
        for (i = 0; i < SR_ARPCACHE_SZ; i++) {
            if ((cache->entries[i].valid) && (difftime(curtime,cache->entries[i].added) > SR_ARPCACHE_TO)) {
                cache->entries[i].valid = 0;
            }
        }

        sr_arpcache_sweepreqs(sr);

        pthread_mutex_unlock(&(cache->lock));
    }

    return NULL;
}
