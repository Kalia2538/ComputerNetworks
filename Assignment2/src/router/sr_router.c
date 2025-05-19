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
  print_hdrs(packet, len);  
  sr_ethernet_hdr_t *eth_hdr = (sr_ethernet_hdr_t *) packet;
  
  // TODO: Do i need to check if the ethernet header is properly formatted?
  switch (ethertype(packet)) {
    case ethertype_arp: { // arp
      if (len < sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t)) {
        fprintf(stderr, "pkt len too short for arp\n");
        return;
      }
      sr_arp_hdr_t *arphdr = (sr_arp_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
      // TODO: make sure the arp is well formatted
      if (ntohs(arphdr->ar_op)  == arp_op_request) {
        // struct sr_if * iface = sr_get_interface(sr, interface);
        
        struct sr_if * iface = sr_get_interface(sr, interface);
        if (iface != NULL && arphdr->ar_tip == iface->ip) { // addressed to us // also redundant tbh
          // allocate space for the newly created reply
          uint8_t * reply = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t));
          if (reply == NULL) {
            fprintf(stderr, "Error: malloc failed while creating ARP reply.\n");
            return;
          }
          // initialize to 0
          memset(reply, 0, sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t));
          create_arp_reply(reply, iface, eth_hdr->ether_shost, arphdr->ar_sip);
          unsigned int length = sizeof(sr_ethernet_hdr_t)+sizeof(sr_arp_hdr_t);          
          printf("packet(reply) to be sent \n");
          print_hdrs(reply, length);
          int check = sr_send_packet(sr, reply, length, iface->name);
          printf("if 0, sending was successfull (allegedly): %d\n", check);
          // free the reply
          free(reply);
        } else {
          printf("target == NULL...dropping the packet\n");
        }
        break;
        
        // no
        // "drop it" --> does that mean do nothing???
      } else if (ntohs(arphdr->ar_op) == arp_op_reply) {
        printf("now we have a reply(allegedly) %u\n", ntohs(arphdr->ar_op));
        printf("looking up %s\n", interface);
        struct sr_if * iface = sr_get_interface(sr, interface);
        // cache the reply
        // check if in the cache already has the reply entry
        struct sr_arpreq * in_cache = sr_arpcache_insert(&sr->cache, arphdr->ar_sha, arphdr->ar_sip);
        if (in_cache != NULL) { // entry already in cache // TODO: Does this need to be outside of the target if statement?
          // check to see who else is waiting on this reply
          struct sr_packet *curr_pkt = in_cache->packets;
          while (curr_pkt != NULL) {
            // TODO: update packet

            sr_ethernet_hdr_t  * update_eth = (sr_ethernet_hdr_t*) curr_pkt->buf;              
            // use buff to extract etherhdr
            memcpy(update_eth->ether_dhost, arphdr->ar_sha, ETHER_ADDR_LEN); // TODO: CHECK THE SIZE VALUE
            // update ether.dhost
            memcpy(update_eth->ether_shost, iface->addr, ETHER_ADDR_LEN);
            // put packet back together

            printf("updated packet\n");
            print_hdrs(curr_pkt->buf, curr_pkt->len);

            // sr_send the packet
            sr_send_packet(sr, curr_pkt->buf, curr_pkt->len, iface->name);

            curr_pkt = curr_pkt->next;
          }
            // send the IP packets waiting on this request
          sr_arpreq_destroy(&sr->cache, in_cache);            
        } 
        
        break;
      }
    }
    case ethertype_ip: { // ip 
      printf("we have an ip packet\n");
      sr_ip_hdr_t *iphdr = (sr_ip_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
      // TODO: Verify length
      // verify checksum
      uint16_t hdr_sum = iphdr->ip_sum;
      iphdr->ip_sum = 0;
      if (cksum(iphdr, sizeof(sr_ip_hdr_t)) != hdr_sum) {
        printf("checksum fail\n");
        return;
      }

      iphdr->ip_sum = hdr_sum;
      
      struct sr_if *iface = get_interface_from_ip(sr, iphdr->ip_dst);

      if (iface != NULL) { // addressed to us
        if (iphdr->ip_p == ip_protocol_icmp) { // it is an ICMP
          sr_icmp_hdr_t * icmp_hdr = (sr_icmp_hdr_t *)(packet + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
          // TO UPDATE: DO WE NEED TO CHECK ICMP CHECKSUM HERE TOO
          
          uint16_t icmp_sum = icmp_hdr->icmp_sum;
          icmp_hdr->icmp_sum = 0;
          if (cksum(icmp_hdr, sizeof(sr_icmp_hdr_t))!= icmp_sum) {
            printf("ICMP cksum fail\n");
          }

          if (icmp_hdr->icmp_type == 8) {
            // malloc size of ethernet header + ip header + icmp header
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
            
            new_hdr_icmp->icmp_type = 0;
            new_hdr_icmp->icmp_sum = 0;
            // update icmp data
            memcpy(new_hdr_icmp->data, iphdr, sizeof(sr_ip_hdr_t) + 8 );


            uint16_t i_sum = cksum(new_hdr_icmp, len - sizeof(sr_ip_hdr_t) - sizeof(sr_ethernet_hdr_t));
            new_hdr_icmp->icmp_sum = i_sum;

            // set the ip header
            sr_ip_hdr_t *new_ip_hdr = (sr_ip_hdr_t *) (icmp_msg + sizeof(sr_ethernet_hdr_t));
            new_ip_hdr->ip_tos = iphdr->ip_tos; // do i need to use memcpy here?
            // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
            new_ip_hdr->ip_len = htons(sizeof(packet) - sizeof(sr_ethernet_hdr_t));
            new_ip_hdr->ip_ttl = INIT_TTL;
            new_ip_hdr->ip_p = ip_protocol_icmp;
            new_ip_hdr->ip_sum = 0;
            new_ip_hdr->ip_src = iface->ip; // DC: is this okay
            new_ip_hdr->ip_dst = iphdr->ip_src;
            uint16_t ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));
            new_ip_hdr->ip_sum = ip_sum;
            // do i need to update new_ip_hdr->id?
            
            // set the ethernet header
            printf("looking up %s\n", interface);
            struct sr_if * our_iface = sr_get_interface(sr, interface);
            
            sr_ethernet_hdr_t *new_eth_hdr = (sr_ethernet_hdr_t*) (icmp_msg);
            memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, ETHER_ADDR_LEN);
            memcpy(new_eth_hdr->ether_shost, our_iface->addr, ETHER_ADDR_LEN);
            new_eth_hdr->ether_type = htons(ethertype_ip);
            
            printf("icmp msg\n");
            print_hdrs(icmp_msg, packet_length);

            
            // send the packet
            
            sr_send_packet(sr, icmp_msg, packet_length, interface);
          }
          return;
        } else { // not icmp
          // send type code 3
          uint8_t * icmp_msg = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
          unsigned int packet_length = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
          memset(icmp_msg, 0, packet_length);

          // set the icmp hdr
          sr_icmp_hdr_t *new_hdr_icmp = (sr_icmp_hdr_t *)(icmp_msg + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
            
          new_hdr_icmp->icmp_type = 3;
          new_hdr_icmp->icmp_code = 3;
          new_hdr_icmp->icmp_sum = 0;
          // update icmp data
          memcpy(new_hdr_icmp->data, iphdr, sizeof(sr_ip_hdr_t) + 8 );


          uint16_t i_sum = cksum(new_hdr_icmp, len - sizeof(sr_ip_hdr_t) - sizeof(sr_ethernet_hdr_t));
          new_hdr_icmp->icmp_sum = i_sum;

          //////////////////////////////////////////

          // set the ip header
          sr_ip_hdr_t *new_ip_hdr = (sr_ip_hdr_t *) (icmp_msg + sizeof(sr_ethernet_hdr_t));
          new_ip_hdr->ip_tos = iphdr->ip_tos; // do i need to use memcpy here?
          // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
          new_ip_hdr->ip_len = htons(sizeof(packet - sizeof(sr_ethernet_hdr_t)));
          new_ip_hdr->ip_ttl = INIT_TTL;
          new_ip_hdr->ip_p = ip_protocol_icmp;
          new_ip_hdr->ip_sum = 0;
          new_ip_hdr->ip_src = iface->ip; // DC: is this okay
          new_ip_hdr->ip_dst = iphdr->ip_src;
          uint16_t ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));
          new_ip_hdr->ip_sum = ip_sum;



          // set the ethernet header
          printf("looking up %s\n", interface);
          struct sr_if * our_iface = sr_get_interface(sr, interface);
          sr_ethernet_hdr_t *new_eth_hdr = (sr_ethernet_hdr_t*) (icmp_msg);
          memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, ETHER_ADDR_LEN);
          memcpy(new_eth_hdr->ether_shost, our_iface->addr, ETHER_ADDR_LEN);
          new_eth_hdr->ether_type = htons(ethertype_ip);
        
          // send the packet
          printf("icmp msg\n");
          print_hdrs(icmp_msg, packet_length);

          sr_send_packet(sr, icmp_msg, packet_length, interface);

          return;
        }
        
      } else { // interface == NULL -- Not addressed to us -- needs to be forwarded

        iphdr->ip_ttl--; // decrement ttl
        if (iphdr->ip_ttl <= 0) { // true if ttl has expired
          // TODO: send ICMP type 11
          // data for ICMP  is the og packet (can copy from above)


          uint8_t * icmp_msg = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
          unsigned int packet_length = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
          memset(icmp_msg, 0, packet_length);

          // set the icmp hdr
          sr_icmp_hdr_t *new_hdr_icmp = (sr_icmp_hdr_t *)(icmp_msg + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
            
          new_hdr_icmp->icmp_type = 11;
          new_hdr_icmp->icmp_code = 0;
          new_hdr_icmp->icmp_sum = 0;
          // update icmp data
          memcpy(new_hdr_icmp->data, iphdr, sizeof(sr_ip_hdr_t) + 8 );


          uint16_t i_sum = cksum(new_hdr_icmp, len - sizeof(sr_ip_hdr_t) - sizeof(sr_ethernet_hdr_t));
          new_hdr_icmp->icmp_sum = i_sum;

          //////////////////////////////////////////

          // set the ip header
          sr_ip_hdr_t *new_ip_hdr = (sr_ip_hdr_t *) (icmp_msg + sizeof(sr_ethernet_hdr_t));
          new_ip_hdr->ip_tos = iphdr->ip_tos; // do i need to use memcpy here?
          // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
          new_ip_hdr->ip_len = htons(sizeof(packet - sizeof(sr_ethernet_hdr_t)));
          new_ip_hdr->ip_ttl = INIT_TTL;
          new_ip_hdr->ip_p = ip_protocol_icmp;
          new_ip_hdr->ip_sum = 0;
          new_ip_hdr->ip_src = iface->ip; // DC: is this okay
          new_ip_hdr->ip_dst = iphdr->ip_src;
          uint16_t ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));
          new_ip_hdr->ip_sum = ip_sum;



          // set the ethernet header
          printf("looking up %s\n", interface);
          struct sr_if * our_iface = sr_get_interface(sr, interface);
          sr_ethernet_hdr_t *new_eth_hdr = (sr_ethernet_hdr_t*) (icmp_msg);
          memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, ETHER_ADDR_LEN);
          memcpy(new_eth_hdr->ether_shost, our_iface->addr, ETHER_ADDR_LEN);
          new_eth_hdr->ether_type = htons(ethertype_ip);

          printf("icmp msg\n");
          print_hdrs(icmp_msg, packet_length);

          sr_send_packet(sr, icmp_msg, packet_length, interface);
          return;
        }
        // recompute checksum
        iphdr->ip_sum = 0;
        iphdr->ip_sum = cksum(iphdr, sizeof(sr_ip_hdr_t));
        ///////////////////////////////////////////////////////


        
        ///////////////////////////////////////////////////////

       
        // find the match dest ip in routing table
        struct sr_rt * match = find_rt_dest(sr, iphdr->ip_dst);
        if (match == NULL) {
          // TODO: send ICMP type 3 code 3
          /////////////////////////////////////////////////


          // TODO: send type code 3
          uint8_t * icmp_msg = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t));
          unsigned int packet_length = sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t) + sizeof(sr_icmp_hdr_t);
          memset(icmp_msg, 0, packet_length);

          // set the icmp hdr
          sr_icmp_hdr_t *new_hdr_icmp = (sr_icmp_hdr_t *)(icmp_msg + sizeof(sr_ethernet_hdr_t) + sizeof(sr_ip_hdr_t));
            
          new_hdr_icmp->icmp_type = 3;
          new_hdr_icmp->icmp_code = 3;
          new_hdr_icmp->icmp_sum = 0;
          // update icmp data
          memcpy(new_hdr_icmp->data, iphdr, sizeof(sr_ip_hdr_t) + 8 );


          uint16_t i_sum = cksum(new_hdr_icmp, len - sizeof(sr_ip_hdr_t) - sizeof(sr_ethernet_hdr_t));
          new_hdr_icmp->icmp_sum = i_sum;

          //////////////////////////////////////////

          // set the ip header
          sr_ip_hdr_t *new_ip_hdr = (sr_ip_hdr_t *) (icmp_msg + sizeof(sr_ethernet_hdr_t));
          new_ip_hdr->ip_tos = iphdr->ip_tos; // do i need to use memcpy here?
          // memcpy(new_ip_hdr->ip_tos, iphdr->ip_tos, 8); // DC: do i put 8 bytes for this?
          new_ip_hdr->ip_len = htons(sizeof(packet - sizeof(sr_ethernet_hdr_t)));
          new_ip_hdr->ip_ttl = INIT_TTL;
          new_ip_hdr->ip_p = ip_protocol_icmp;
          new_ip_hdr->ip_sum = 0;
          new_ip_hdr->ip_src = iface->ip; // DC: is this okay
          new_ip_hdr->ip_dst = iphdr->ip_src;
          uint16_t ip_sum = cksum(new_ip_hdr, sizeof(sr_ip_hdr_t));
          new_ip_hdr->ip_sum = ip_sum;



          // set the ethernet header
          printf("looking up %s\n", interface);
          struct sr_if * our_iface = sr_get_interface(sr, interface);
          sr_ethernet_hdr_t *new_eth_hdr = (sr_ethernet_hdr_t*) (icmp_msg);
          memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, ETHER_ADDR_LEN);
          memcpy(new_eth_hdr->ether_shost, our_iface->addr, ETHER_ADDR_LEN);
          new_eth_hdr->ether_type = htons(ethertype_ip);

          printf("icmp msg\n");
          print_hdrs(icmp_msg, packet_length);

          sr_send_packet(sr, icmp_msg, packet_length, interface);


          return;
        } else {

          struct sr_rt * match = find_rt_dest(sr, iphdr->ip_dst);
          uint32_t nh_ip = match->gw.s_addr;
          if (nh_ip == 0) { // DC this
            nh_ip = iphdr->ip_dst;
          }

          printf("looking up %s\n", interface);
          struct sr_if * o_face = sr_get_interface(sr, match->interface);
          // TODO: ERROR CHECK HERE?


          struct sr_arpentry * curr = sr_arpcache_lookup(&sr->cache, nh_ip);
          if (curr != NULL) { // info in the cache

            // quick update of eth hdr
            memcpy(eth_hdr->ether_dhost, curr->mac, ETHER_ADDR_LEN);
            memcpy(eth_hdr->ether_shost, o_face->addr, ETHER_ADDR_LEN);
            sr_send_packet(sr, packet, len, o_face->name); // DC: THESE VALUES
            return;
          } else { // info not in the cache
            // DC: sending an arp request???
            struct sr_arpreq * req = sr_arpcache_queuereq(&sr->cache, nh_ip, packet, len, match->interface);
            handle_arpreq(sr, req);
          }
        }

      }

    }
    break;
  }
  // need to determine if this is an ip or arp packet
  

} /* end sr_handlepacket */

void create_arp_reply(uint8_t * packet, struct sr_if * interface, unsigned char * tha, uint32_t tip) {
  sr_ethernet_hdr_t * eth_hdr = (sr_ethernet_hdr_t * ) packet;
  memcpy(eth_hdr->ether_dhost, tha, 6);
  memcpy(eth_hdr->ether_shost, interface->addr, 6);
  eth_hdr->ether_type = htons(ethertype_arp);
  
  sr_arp_hdr_t * arp_hdr = (sr_arp_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
  arp_hdr->ar_hrd = htons(arp_hrd_ethernet);
  arp_hdr->ar_pro = htons(ethertype_ip);
  arp_hdr->ar_hln = ETHER_ADDR_LEN;
  arp_hdr->ar_pln = sizeof(uint32_t);
  arp_hdr->ar_op = htons(arp_op_reply);
  memcpy(arp_hdr->ar_sha, interface->addr, ETHER_ADDR_LEN);
  arp_hdr->ar_sip = interface->ip;
  memcpy(arp_hdr->ar_tha, tha, ETHER_ADDR_LEN);
  arp_hdr->ar_tip = tip;  




  // Debug print statements to verify the packet contents
  printf("Created ARP reply packet:\n");
  printf("Ethernet Header:\n");
  printf("  Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
         eth_hdr->ether_dhost[0], eth_hdr->ether_dhost[1], eth_hdr->ether_dhost[2],
         eth_hdr->ether_dhost[3], eth_hdr->ether_dhost[4], eth_hdr->ether_dhost[5]);
  printf("  Src MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
         eth_hdr->ether_shost[0], eth_hdr->ether_shost[1], eth_hdr->ether_shost[2],
         eth_hdr->ether_shost[3], eth_hdr->ether_shost[4], eth_hdr->ether_shost[5]);
  printf("  EtherType: 0x%04x\n", ntohs(eth_hdr->ether_type));
  

  printf("ARP Header:\n");
  printf("  Hardware type: %d\n", ntohs(arp_hdr->ar_hrd));
  printf("  Protocol type: 0x%04x\n", ntohs(arp_hdr->ar_pro));
  printf("  Hardware size: %d\n", arp_hdr->ar_hln);
  printf("  Protocol size: %d\n", arp_hdr->ar_pln);
  printf("  Opcode: %d\n", ntohs(arp_hdr->ar_op));
  printf("  Sender MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
         arp_hdr->ar_sha[0], arp_hdr->ar_sha[1], arp_hdr->ar_sha[2],
         arp_hdr->ar_sha[3], arp_hdr->ar_sha[4], arp_hdr->ar_sha[5]);
  struct in_addr sip_addr = { arp_hdr->ar_sip };
  printf("  Sender IP: %s\n", inet_ntoa(sip_addr));
  printf("  Target MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
         arp_hdr->ar_tha[0], arp_hdr->ar_tha[1], arp_hdr->ar_tha[2],
         arp_hdr->ar_tha[3], arp_hdr->ar_tha[4], arp_hdr->ar_tha[5]);
  struct in_addr tip_addr = { arp_hdr->ar_tip };
  printf("  Target IP: %s\n", inet_ntoa(tip_addr));
}




// longest prefix match
struct sr_rt* find_rt_dest(struct sr_instance * sr, uint32_t dest_ip) {
  struct sr_rt * curr = sr->routing_table;
  struct sr_rt * longest_prefiix_match; // the rt struct of the 
  uint32_t lpm_mask = 0; // the current longest mask;
  while (curr != NULL) {
    // destIP & mask == dest
    if ((dest_ip & curr->mask.s_addr) == (curr->mask.s_addr & curr->dest.s_addr)) { // we found a match
      if (curr->mask.s_addr > lpm_mask) {
        lpm_mask = curr->mask.s_addr;
        longest_prefiix_match = curr;
      }
    }
    curr = curr->next;
  }

  if (lpm_mask == 0) {
    return NULL;
  }

  return longest_prefiix_match;
}



/* Add any additional helper methods here & don't forget to also declare
them in sr_router.h.






If you use any of these methods in sr_arpcache.c, you must also forward declare
them in sr_arpcache.h to avoid circular dependencies. Since sr_router
already imports sr_arpcache.h, sr_arpcache cannot import sr_router.h -KM */

// void print_eth_hdr(sr_ethernet_hdr_t * hdr) {
//   printf("Destination MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
//     hdr->ether_dhost[0], hdr->ether_dhost[1], hdr->ether_dhost[2],
//     hdr->ether_dhost[3], hdr->ether_dhost[4], hdr->ether_dhost[5]);
// }

// void print_arp_hdr(const struct sr_arp_hdr * hdr) {
//   printf("=== ARP Header ===\n");

//   printf("Hardware Type: %u\n", ntohs(hdr->ar_hrd));
//   printf("Protocol Type: 0x%04x\n", ntohs(hdr->ar_pro));
//   printf("Hardware Addr Len: %u\n", hdr->ar_hln);
//   printf("Protocol Addr Len: %u\n", hdr->ar_pln);

//   unsigned short opcode = ntohs(hdr->ar_op);
//   printf("Opcode: %u (%s)\n", opcode,
//          opcode == 1 ? "Request" :
//          opcode == 2 ? "Reply" : "Unknown");

//   // Sender MAC
//   printf("Sender MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
//          hdr->ar_sha[0], hdr->ar_sha[1], hdr->ar_sha[2],
//          hdr->ar_sha[3], hdr->ar_sha[4], hdr->ar_sha[5]);

//   // Sender IP
//   struct in_addr sender_ip;
//   sender_ip.s_addr = hdr->ar_sip;
//   printf("Sender IP: %s\n", inet_ntoa(sender_ip));

//   // Target MAC
//   printf("Target MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
//          hdr->ar_tha[0], hdr->ar_tha[1], hdr->ar_tha[2],
//          hdr->ar_tha[3], hdr->ar_tha[4], hdr->ar_tha[5]);

//   // Target IP
//   struct in_addr target_ip;
//   target_ip.s_addr = hdr->ar_tip;
//   printf("Target IP: %s\n", inet_ntoa(target_ip));
// }

// void print_ip_header(const struct sr_ip_hdr * ip) {
//   printf("=== IP Header ===\n");

//   unsigned int version = ip->ip_v;
//   unsigned int header_length = ip->ip_hl * 4;  // ip_hl is in 32-bit words

//   printf("Version: %u\n", version);
//   printf("Header Length: %u bytes\n", header_length);
//   printf("Type of Service: 0x%02x\n", ip->ip_tos);
//   printf("Total Length: %u\n", ntohs(ip->ip_len));
//   printf("Identification: 0x%04x\n", ntohs(ip->ip_id));
  
//   unsigned short frag_flags = ntohs(ip->ip_off);
//   printf("Fragment Offset: 0x%04x\n", frag_flags);
//   printf("  - RF: %s\n", (frag_flags & IP_RF) ? "Set" : "Not Set");
//   printf("  - DF: %s\n", (frag_flags & IP_DF) ? "Set" : "Not Set");
//   printf("  - MF: %s\n", (frag_flags & IP_MF) ? "Set" : "Not Set");

//   printf("Time to Live (TTL): %u\n", ip->ip_ttl);

//   unsigned int protocol = ip->ip_p;
//   printf("Protocol: %u (%s)\n", protocol,
//          protocol == 1 ? "ICMP" :
//          protocol == 6 ? "TCP" :
//          protocol == 17 ? "UDP" : "Other");

//   printf("Checksum: 0x%04x\n", ntohs(ip->ip_sum));

//   // Source IP address
//   struct in_addr src_ip;
//   src_ip.s_addr = ip->ip_src;
//   printf("Source IP: %s\n", inet_ntoa(src_ip));

//   // Destination IP address
//   struct in_addr dst_ip;
//   dst_ip.s_addr = ip->ip_dst;
//   printf("Destination IP: %s\n", inet_ntoa(dst_ip));
// }


// void print_icmp_hdr(const struct sr_icmp_hdr * icmp_hdr) {
//     printf("=== ICMP Header ===\n");

//     // Print ICMP Type and Code
//     printf("Type: %u\n", icmp_hdr->icmp_type);
//     printf("Code: %u\n", icmp_hdr->icmp_code);

//     // Print Checksum
//     printf("Checksum: 0x%04x\n", ntohs(icmp_hdr->icmp_sum));

//     // Print unused field
//     printf("Unused: 0x%08x\n", ntohl(icmp_hdr->unused));

//     // Print ICMP data (you may want to print it in hex format or ASCII based on your needs)
//     printf("Data: ");
//     for (int i = 0; i < ICMP_DATA_SIZE; i++) {
//         printf("%02x ", icmp_hdr->data[i]);
//     }
//     printf("\n");
// }



