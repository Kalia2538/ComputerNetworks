/**********************************************************************
 * file:  sr_router.c
 *
 * Description:
 *
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing.
 *
 **********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_arpcache.h"
#include "sr_utils.h"

/*---------------------------------------------------------------------
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 *
 *---------------------------------------------------------------------*/

void sr_init(struct sr_instance* sr)
{
    /* REQUIRES */
    assert(sr);

    /* Initialize cache and cache cleanup thread */
    sr_arpcache_init(&(sr->cache));

    pthread_attr_init(&(sr->attr));
    pthread_attr_setdetachstate(&(sr->attr), PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setscope(&(sr->attr), PTHREAD_SCOPE_SYSTEM);
    pthread_t thread;

    pthread_create(&thread, &(sr->attr), sr_arpcache_timeout, sr);

    /* Add initialization code here! */

} /* -- sr_init -- */

/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr,
        uint8_t * packet/* lent */,
        unsigned int len,
        char* interface/* lent */)
{
  /* REQUIRES */
  assert(sr);
  assert(packet);
  assert(interface);

  printf("*** -> Received packet of length %d \n",len);

  /* fill in code here */
  
  // function passes in a packet
  // casting to packet struct
  // struct sr_packet *sr_pack = (struct sr_packet *) packet;
  int result = packet_type(packet);
  assert(result != 0); // ensures the packet is ip or arp
  sr_ethernet_hdr_t *eth_hdr = (sr_ethernet_hdr_t *) packet;
  
  // TODO: Do i need to check if the ethernet header is properly formatted?
    // ex: do i need to check the src and dest addrs?

  switch (result) {
    case 1: // arp
      sr_arp_hdr_t *arphdr = (sr_arp_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
      // TODO: make sure the arp is well formatted
      if (arphdr->ar_op  == arp_op_request) {
        // check if target ip in our router      
        struct sr_if *target = get_interface_from_ip(sr, arphdr->ar_tip);
          //yes
        if (target != NULL) {
          // allocate space for the newly created reply
          uint8_t * reply = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t));
          // TODO: add error checking for the malloc call

          // instantiate ethernet header
          sr_ethernet_hdr_t *new_eth_hdr = (sr_ethernet_hdr_t *)(packet);
          memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, ETHER_ADDR_LEN);
          memcpy(new_eth_hdr->ether_shost, target->addr, ETHER_ADDR_LEN);
          new_eth_hdr->ether_type = htons(ethertype_arp);

          // copy over the ethernet header
          memcpy(reply, new_eth_hdr, sizeof(sr_ethernet_hdr_t)); // TODO: double check the math here

          // instantiate arp header
          sr_arp_hdr_t * new_arp_hdr = (sr_arp_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
          new_arp_hdr->ar_hrd = htons(arp_hrd_ethernet);
          new_arp_hdr->ar_pro = htons(ethertype_ip);
          new_arp_hdr->ar_hln = arphdr->ar_hln; // assuming this variable should be the same as last time
          new_arp_hdr->ar_pln = arphdr->ar_pln;
          new_arp_hdr->ar_op = htons(arp_op_reply);
          memcpy(new_arp_hdr->ar_sha, target->addr, ETHER_ADDR_LEN);
          new_arp_hdr->ar_sip = target->ip;
          memcpy(new_arp_hdr->ar_tha, eth_hdr->ether_shost, ETHER_ADDR_LEN);
          new_arp_hdr->ar_tip = arphdr->ar_sip;

          // copy over the new arp header
          memcpy(reply + sizeof(sr_ethernet_hdr_t), new_arp_hdr, sizeof(sr_arp_hdr_t));
          unsigned int length = sizeof(sr_ethernet_hdr_t)+sizeof(sr_arp_hdr_t);

          // send the packet
          sr_send_packet(sr, reply, length, target->name);
          // free the reply
          free(reply);
        }
        // no
        // "drop it" --> does that mean do nothing???
      } else if (arphdr->ar_op == arp_op_reply) {
        // do reply stuff
        // check if ip target is one of our ip addresses
        struct sr_if *target = get_interface_from_ip(sr, arphdr->ar_tip);
        if (target != NULL) {
          // cache the reply
          // check if in the cache already has the reply entry
          struct sr_arpreq * in_cache = sr_arpcache_insert(&sr->cache, arphdr->ar_tha, target->ip);
          if (in_cache != NULL) { // entry already in cache // TODO: Does this need to be outside of the target if statement?
            // check to see who else is waiting on this reply
            // traverse req.packets
            struct sr_packet *curr_pkt = in_cache->packets;
            unsigned char mac[6];
            memcpy(mac, arphdr->ar_tha, 1);
            while (curr_pkt != NULL) {
              // TODO: update packet
              // get srpkt.buff
              sr_ethernet_hdr_t  * update_eth = (sr_ethernet_hdr_t*) curr_pkt->buf;              
              // use buff to extract etherhdr
              memcpy(update_eth->ether_dhost, mac, 1); // TODO: CHECK THE SIZE VALUE
              // update ether.dhost
              // put packet back together
              memcpy(curr_pkt->buf, update_eth, sizeof(sr_ethernet_hdr_t));
              // sr_send the packet
              sr_send_packet(sr,curr_pkt->buf, sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t), curr_pkt->iface);
              // TODO: send packet
              curr_pkt = curr_pkt->next;
            }
              // send the IP packets waiting on this request
            sr_arpreq_destroy(&sr->cache, in_cache);            
            // free(in_cache); // TODO: double check if this is needed or not
          } 
        }
      } else {
        // not a properly formatted arp header
        return;
      }
    case 9: // ip
      
      sr_ip_hdr_t *iphdr = (sr_ip_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
      // TODO: Verify length
      // verify checksum
      assert(cksum(iphdr, iphdr->ip_len) == iphdr->ip_sum); // TODO: make sure these are the correct parameters
      struct sr_if *interface = get_interface_from_ip(sr, iphdr->ip_dst);
      if (interface != NULL) { // addressed to us
        if (iphdr->ip_p == ip_protocol_icmp) { // it is an ICMP
          sr_icmp_hdr_t * icmp_hdr = (sr_icmp_hdr_t *)(packet + sizeof(sr_ethernet_hdr_t));
          if (icmp_hdr->icmp_type == 8) {
            // malloc size of ethernet header + ip header + icmp header
            uint8_t * icmp_msg = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
            unsigned int packet_length = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);

            // set the icmp hdr
            sr_icmp_hdr_t *new_hdr_icmp;
            new_hdr_icmp->icmp_type = 0;
            new_hdr_icmp->icmp_sum = 0;
            uint16_t i_sum = cksum(new_hdr_icmp, sizeof(sr_icmp_hdr_t));
            new_hdr_icmp->icmp_sum = i_sum;

            // set the ip header
            sr_ip_hdr_t *new_ip_hdr;
            new_ip_hdr->ip_tos = iphdr->ip_tos; // do i need to use memcpy here?
            // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
            new_ip_hdr->ip_len = htons(sizeof(packet - sizeof(sr_ethernet_hdr_t)));
            new_ip_hdr->ip_ttl = INIT_TTL;
            new_ip_hdr->ip_p = ip_protocol_icmp;
            new_ip_hdr->ip_sum = 0;
            new_ip_hdr->ip_src = iphdr->ip_dst; // DC: is this okay
            new_ip_hdr->ip_dst = iphdr->ip_src;
            uint16_t ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));
            new_ip_hdr->ip_sum = ip_sum;
            // do i need to update new_ip_hdr->id?
            
            // set the ethernet header
            sr_ethernet_hdr_t *new_eth_hdr;
            memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, ETHER_ADDR_LEN);
            memcpy(new_eth_hdr->ether_shost, eth_hdr->ether_dhost, ETHER_ADDR_LEN);
            new_eth_hdr->ether_type = htons(ethertype_ip);
            
            // construct the packet
            memcpy(icmp_msg, new_eth_hdr, sizeof(sr_ethernet_hdr_t));
            memcpy(icmp_msg + sizeof(sr_ethernet_hdr_t), new_ip_hdr, sizeof(sr_ip_hdr_t));
            memcpy(icmp_msg + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t), new_hdr_icmp, sizeof(sr_icmp_hdr_t));

            // find dest interface
            struct sr_if *new_dest = get_interface_from_ip(sr, iphdr->ip_dst);
            // send the packet
            sr_send_packet(sr, icmp_msg, packet_length, new_dest->name);
          }
          return;
        } else { // not icmp
          // TODO: sent type code 3
          uint8_t * icmp_msg = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
          unsigned int packet_length = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);

          // set the icmp hdr
          sr_icmp_hdr_t *hdr_icmp;
          hdr_icmp->icmp_type = 3;
          hdr_icmp->icmp_code = 3;
          memcpy(hdr_icmp->data, (uint8_t *)iphdr, sizeof(iphdr)); // DC: make sure the 2nd and 3rd params are correct
          hdr_icmp->icmp_sum = 0;
          uint16_t icmp_sum = cksum(hdr_icmp, sizeof(sr_icmp_hdr_t));
          hdr_icmp->icmp_sum = icmp_sum;

          // set the ip header
          sr_ip_hdr_t *new_ip_hdr;
          new_ip_hdr->ip_tos = iphdr->ip_tos; // do i need to use memcpy here?
          // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
          new_ip_hdr->ip_len = htons(sizeof(packet - sizeof(sr_ethernet_hdr_t)));
          new_ip_hdr->ip_ttl = INIT_TTL;
          new_ip_hdr->ip_p = ip_protocol_icmp;
          new_ip_hdr->ip_sum = 0;
          new_ip_hdr->ip_src = iphdr->ip_dst; // DC: is this okay
          new_ip_hdr->ip_dst = iphdr->ip_src;
          uint16_t ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));
          new_ip_hdr->ip_sum = ip_sum;
          // do i need to update new_ip_hdr->id?
          
          // set the ethernet header
          sr_ethernet_hdr_t *new_eth_hdr;
          memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, ETHER_ADDR_LEN);
          memcpy(new_eth_hdr->ether_shost, eth_hdr->ether_dhost, ETHER_ADDR_LEN);
          new_eth_hdr->ether_type = htons(ethertype_ip);
          
          // construct the packet
          memcpy(icmp_msg, new_eth_hdr, sizeof(sr_ethernet_hdr_t));
          memcpy(icmp_msg + sizeof(sr_ethernet_hdr_t), new_ip_hdr, sizeof(sr_ip_hdr_t));
          memcpy(icmp_msg + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t), hdr_icmp, sizeof(sr_icmp_hdr_t));

          // find dest interface
          struct sr_if *new_dest = get_interface_from_ip(sr, iphdr->ip_dst);
          // send the packet
          sr_send_packet(sr, icmp_msg, packet_length, new_dest->name);

          return;
        }
        
      } else { // interface == NULL -- Not addressed to us -- needs to be forwarded
        iphdr->ip_ttl--; // decrement ttl
        if (iphdr->ip_ttl <= 0) { // true if ttl has expired
          // TODO: send ICMP type 11
            // data for ICMP  is the og packet (can copy from above)
          uint8_t * icmp_msg = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
          unsigned int packet_length = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);

          // set the icmp hdr
          sr_icmp_hdr_t *hdr_icmp;
          hdr_icmp->icmp_type = 11;
          hdr_icmp->icmp_code = 0;
          memcpy(hdr_icmp->data, (uint8_t *)iphdr, sizeof(iphdr)); // DC: make sure the 2nd and 3rd params are correct
          hdr_icmp->icmp_sum = 0;
          uint16_t icmp_sum = cksum(hdr_icmp, sizeof(sr_icmp_hdr_t));
          hdr_icmp->icmp_sum = icmp_sum;

          // set the ip header
          sr_ip_hdr_t *new_ip_hdr;
          new_ip_hdr->ip_tos = iphdr->ip_tos; // do i need to use memcpy here?
          // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
          new_ip_hdr->ip_len = htons(sizeof(packet - sizeof(sr_ethernet_hdr_t)));
          new_ip_hdr->ip_ttl = INIT_TTL;
          new_ip_hdr->ip_p = ip_protocol_icmp;
          new_ip_hdr->ip_sum = 0;
          new_ip_hdr->ip_src = iphdr->ip_dst; // DC: is this okay
          new_ip_hdr->ip_dst = iphdr->ip_src;
          uint16_t ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));
          new_ip_hdr->ip_sum = ip_sum;
          // do i need to update new_ip_hdr->id?
          
          // set the ethernet header
          sr_ethernet_hdr_t *new_eth_hdr;
          // memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, ETHER_ADDR_LEN); -- need the nexthop mac
          memcpy(new_eth_hdr->ether_shost, eth_hdr->ether_dhost, ETHER_ADDR_LEN);
          new_eth_hdr->ether_type = htons(ethertype_ip);
          
          // construct the packet
          memcpy(icmp_msg, new_eth_hdr, sizeof(sr_ethernet_hdr_t));
          memcpy(icmp_msg + sizeof(sr_ethernet_hdr_t), new_ip_hdr, sizeof(sr_ip_hdr_t));
          memcpy(icmp_msg + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t), hdr_icmp, sizeof(sr_icmp_hdr_t));

          // find dest interface
          struct sr_if *new_dest = get_interface_from_ip(sr, iphdr->ip_dst);
          // send the packet
          sr_send_packet(sr, icmp_msg, packet_length, new_dest->name); //dc: do i free the packet that was messed up?
          return;
        }
        // recompute checksum
        //  todo: make sure we dont need this - int new_sum = cksum(iphdr, iphdr->ip_len); // DC: do i need to change csum to 0 b4 doing this???
        // find the match dest ip in routing table
        struct sr_rt * match = find_rt_dest(sr, iphdr->ip_dst);
        if (match == NULL) {
          // TODO: send ICMP type 3 code 3

          // malloc size of ethernet header + ip header + icmp header
          uint8_t * icmp_msg = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
          unsigned int packet_length = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);

          // set the icmp hdr
          sr_icmp_hdr_t *new_hdr_icmp;
          new_hdr_icmp->icmp_type = 3;
          new_hdr_icmp->icmp_code = 3;
          new_hdr_icmp->icmp_sum = 0;
          uint16_t i_sum = cksum(new_hdr_icmp, sizeof(sr_icmp_hdr_t));
          new_hdr_icmp->icmp_sum = i_sum;

          // set the ip header
          sr_ip_hdr_t *new_ip_hdr;
          new_ip_hdr->ip_tos = iphdr->ip_tos; // do i need to use memcpy here?
          // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
          new_ip_hdr->ip_len = htons(sizeof(packet - sizeof(sr_ethernet_hdr_t)));
          new_ip_hdr->ip_ttl = INIT_TTL;
          new_ip_hdr->ip_p = ip_protocol_icmp;
          new_ip_hdr->ip_sum = 0;
          new_ip_hdr->ip_src = iphdr->ip_dst; // DC: is this okay
          new_ip_hdr->ip_dst = iphdr->ip_src;
          uint16_t ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));
          new_ip_hdr->ip_sum = ip_sum;
          // do i need to update new_ip_hdr->id?
          
          // set the ethernet header
          sr_ethernet_hdr_t *new_eth_hdr;
          memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, ETHER_ADDR_LEN);
          memcpy(new_eth_hdr->ether_shost, eth_hdr->ether_dhost, ETHER_ADDR_LEN);
          new_eth_hdr->ether_type = htons(ethertype_ip);
          
          // construct the packet
          memcpy(icmp_msg, new_eth_hdr, sizeof(sr_ethernet_hdr_t));
          memcpy(icmp_msg + sizeof(sr_ethernet_hdr_t), new_ip_hdr, sizeof(sr_ip_hdr_t));
          memcpy(icmp_msg + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t), new_hdr_icmp, sizeof(sr_icmp_hdr_t));

          // find dest interface
          struct sr_if *new_dest = get_interface_from_ip(sr, new_ip_hdr->ip_dst);
          // send the packet
          sr_send_packet(sr, icmp_msg, packet_length, new_dest->name);


          return;
        } else {
          struct sr_arpentry * curr = sr_arpcache_lookup(&sr->cache, iphdr->ip_dst);
          if (curr != NULL) { // info in the cache
            // make a new packet to send
            uint8_t * pkt = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
            // make ethernet header
            sr_ethernet_hdr_t * new_eth;
            memcpy(new_eth->ether_shost, eth_hdr->ether_dhost, 1); // DC: make sure the size is valid
            new_eth->ether_type = htons(ethertype_ip);
            // make ip header
            sr_ip_hdr_t * new_ip;
            new_ip->ip_tos = iphdr->ip_tos; // do i need to use memcpy here?
            // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
            new_ip->ip_len = htons(sizeof(packet - sizeof(sr_ethernet_hdr_t)));
            new_ip->ip_ttl = INIT_TTL;
            new_ip->ip_p = iphdr->ip_p;
            new_ip->ip_sum = 0;
            new_ip->ip_src = iphdr->ip_dst; // DC: is this okay
            new_ip->ip_dst = curr->ip;
            uint16_t ip_sum = cksum(new_ip, sizeof(sr_ip_hdr_t));
            new_ip->ip_sum = ip_sum;
            // do i need to update new_ip_hdr->id?
            // combine

            memcpy(pkt, eth_hdr, sizeof(sr_ethernet_hdr_t));
            memcpy(pkt + sizeof(sr_ethernet_hdr_t), new_ip, sizeof(sr_ip_hdr_t));
            // send the packet
            sr_send_packet(sr,(uint8_t *) packet, sizeof(uint8_t), get_interface_from_ip(sr, match->dest.s_addr)->name);
            free(packet);
            return;
          } else { // info not in the cache
            // check through requests
            struct sr_arpreq * curr_req = sr->cache.requests;
            while(curr_req != NULL) {
              if (curr_req->ip == match->dest.s_addr) { 
                // updtae sent time
                time_t now;
                time(&now);
                curr_req->sent = now;
                // update times sent
                curr_req->times_sent++;     
                // create packet
                uint8_t * pkt = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
                // make ethernet header
                sr_ethernet_hdr_t * new_eth;
                memcpy(new_eth->ether_shost, eth_hdr->ether_dhost, 1); // DC: make sure the size is valid
                new_eth->ether_type = htons(ethertype_ip);
                // make ip header
                sr_ip_hdr_t * new_ip;
                new_ip->ip_tos = iphdr->ip_tos; // do i need to use memcpy here?
                // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
                new_ip->ip_len = htons(sizeof(packet - sizeof(sr_ethernet_hdr_t)));
                new_ip->ip_ttl = INIT_TTL;
                new_ip->ip_p = iphdr->ip_p;
                new_ip->ip_sum = 0;
                new_ip->ip_src = iphdr->ip_dst; // DC: is this okay
                new_ip->ip_dst = curr->ip;
                uint16_t ip_sum = cksum(new_ip, sizeof(sr_ip_hdr_t));
                new_ip->ip_sum = ip_sum;
                // do i need to update new_ip_hdr->id?
                // combine

                memcpy(pkt, eth_hdr, sizeof(sr_ethernet_hdr_t));
                memcpy(pkt + sizeof(sr_ethernet_hdr_t), new_ip, sizeof(sr_ip_hdr_t));
                // add packet to requests 
                struct sr_packet * newsrpkt;    
                newsrpkt->buf = (uint8_t *) pkt;
                newsrpkt->len = sizeof(sr_ip_hdr_t) + sizeof(sr_ethernet_hdr_t);     
                newsrpkt->iface = get_interface_from_ip(sr, curr_req->ip)->name;
                newsrpkt->next = NULL;
                // traverse to end of pkts
                struct sr_packet * cur_pkt = curr_req->packets;
                while (cur_pkt->next != NULL) {
                  cur_pkt = cur_pkt->next;
                }
                // add paccket to requests
                cur_pkt->next = newsrpkt;
                return;
              }
            }
            // need to create a new arp request
            uint8_t *pkt = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
            // make ethernet header
            sr_ethernet_hdr_t * new_eth;
            memcpy(new_eth->ether_shost, eth_hdr->ether_dhost, 1); // DC: make sure the size is valid
            new_eth->ether_type = htons(ethertype_ip);
            // make ip header
            sr_ip_hdr_t * new_ip;
            new_ip->ip_tos = iphdr->ip_tos; // do i need to use memcpy here?
            // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
            new_ip->ip_len = htons(sizeof(packet - sizeof(sr_ethernet_hdr_t)));
            new_ip->ip_ttl = INIT_TTL;
            new_ip->ip_p = iphdr->ip_p;
            new_ip->ip_sum = 0;
            new_ip->ip_src = iphdr->ip_dst; // DC: is this okay
            new_ip->ip_dst = curr->ip;
            uint16_t ip_sum = cksum(new_ip, sizeof(sr_ip_hdr_t));
            new_ip->ip_sum = ip_sum;
            // do i need to update new_ip_hdr->id?
            // combine

            memcpy(pkt, eth_hdr, sizeof(sr_ethernet_hdr_t));
            memcpy(pkt + sizeof(sr_ethernet_hdr_t), new_ip, sizeof(sr_ip_hdr_t));
            // add packet to requests 
            struct sr_packet * newsrpkt;    
            newsrpkt->buf = (uint8_t *) pkt;
            newsrpkt->len = sizeof(sr_ip_hdr_t) + sizeof(sr_ethernet_hdr_t);     
            newsrpkt->iface = get_interface_from_ip(sr, new_ip->ip_dst)->name;
            newsrpkt->next = NULL;

            struct sr_arpreq * new_req;
            new_req->ip = new_ip->ip_dst;
            time_t now;
            time(&now);
            new_req->sent = now;
            new_req->times_sent = 1;
            new_req->packets = newsrpkt;
            new_req->next = NULL;

            if (sr->cache.requests == NULL) {
              sr->cache.requests = new_req;
            } else {
              struct sr_arpreq * cur_cachereq = sr->cache.requests;
              while (cur_cachereq->next != NULL) {
                cur_cachereq = cur_cachereq->next;
              }
              cur_cachereq->next = new_req;
            }
          }
        }
          // no match 
            // send ICMP type 3 code 0
          // match
            // entry found
              // send packet to next hop
            // entry not found
              // send arp request for next hop ip
              // add packet to queue (look for ip in cache.requests)
      }

      break;
  }
  // need to determine if this is an ip or arp packet
  

} /* end sr_handlepacket */


/*  determines if the given packet is arp/ip/other
  if returns
    0: not arp or ip
    1: arp (I picked 1 since 'a' is the first letter of the alphabet)
    9: ip (I picked 9 since 'i' is the 9th letter of the alphabet)
*/
int packet_type(uint8_t * packet) {
  sr_ethernet_hdr_t * hdr = (sr_ethernet_hdr_t *) (packet);
  if (hdr->ether_type == ethertype_arp) { // arp packet
    return 1;
  } else if (hdr->ether_type == ethertype_ip) { // ip packet
    return 9;
  } else {
    return 0; // none of the above
  }
}

// uint8_t create_imcp(int type, int code, uint16_t csum, uint8_t data) {
//   // malloc a 
// }

// longest prefix match
struct sr_rt* find_rt_dest(struct sr_instance * sr, uint32_t dest_ip) {
  struct sr_rt * curr = sr->routing_table;
  struct sr_rt * longest_prefiix_match; // the rt struct of the 
  uint32_t lpm_mask = 0; // the current longest mask;
  while (curr != NULL) {
    // destIP & mask == dest
    if (dest_ip == (curr->mask.s_addr & curr->dest.s_addr)) { // we found a match
      if (curr->mask.s_addr > lpm_mask) {
        lpm_mask = curr->mask.s_addr;
        longest_prefiix_match = curr;
      }
    }
    curr = curr->next;
  }

  if (lpm_mask != 0) {
    return NULL;
  }

  return longest_prefiix_match;
}

// struct sr_packet * make_ip_pkt(struct sr_instance, )



// int validate_arp_hdr (sr_arp_hdr_t arphdr) {
//   if ()
//   assert(arphdr.ar_hrd = );
//   // assert ar_hrd = ethernet
//   // assert ar_pro = ip
//   // assert ar_hln = 1 byte
//   // assert ar_pln = //
//   get_interface_from_ip()


// }



/* Add any additional helper methods here & don't forget to also declare
them in sr_router.h.






If you use any of these methods in sr_arpcache.c, you must also forward declare
them in sr_arpcache.h to avoid circular dependencies. Since sr_router
already imports sr_arpcache.h, sr_arpcache cannot import sr_router.h -KM */
