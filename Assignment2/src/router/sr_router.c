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
  struct sr_packet *sr_pack = (struct sr_packet *) packet;
  int result = packet_type(packet);
  assert(result != 0); // ensures the packet is ip or arp
  sr_ethernet_hdr_t *eth_hdr = (struct sr_ethernet_hdr_t *) packet;
  
  // TODO: Do i need to check if the ethernet header is properly formatted?
    // ex: do i need to check the src and dest addrs?

  switch (result) {
    case 1: // apr
      // do stuff
      sr_arp_hdr_t *arphdr = (sr_arp_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
      // TODO: make sure the rp

      if (arphdr->ar_op  == arp_op_request) {
        // check if target ip in our router      
        struct sr_if *target = get_interface_from_ip(sr, arphdr->ar_tip);
          //yes
          if (target != NULL) {
            // allocate space for the newly created reply
            uint8_t reply = (uint8_t *)malloc(sizeof(sr_ethernet_hdr_t) + sizeof(sr_arp_hdr_t));
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
          }
          // no
            // "drop it" --> does that mean do nothing???
      } else if (arphdr->ar_op == arp_op_reply) {
        // do reply stuff
      } else {
        // not a properly formatted arp header
        return;
      }
    case 9: // ip
      // do stuff
      sr_ip_hdr_t *iphdr = (sr_ip_hdr_t*)(packet + sizeof(sr_ethernet_hdr_t));
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
int packet_type(uint8_t packet) {
  struct  sr_ethernet_hdr * hdr = (struct sr_ethernet_hdr *) packet;
  if (hdr->ether_type == ethertype_arp) { // arp packet
    return 1;
  } else if (hdr->ether_type == ethertype_ip) { // ip packet
    return 9;
  } else {
    return 0; // none of the above
  }
}


// int validate_arp_hdr (sr_arp_hdr_t arphdr) {
//   if ()
//   assert(arphdr.ar_hrd = );
//   // assert ar_hrd = ethernet
//   // assert ar_pro = ip
//   // assert ar_hln = 1 byte
//   // assert ar_pln = //
//   get_interface_from_ip()


// }

struct sr_arp_hdr
{
    unsigned short  ar_hrd;             /* format of hardware address   */
    unsigned short  ar_pro;             /* format of protocol address   */
    unsigned char   ar_hln;             /* length of hardware address   */
    unsigned char   ar_pln;             /* length of protocol address   */
    unsigned short  ar_op;              /* ARP opcode (command)         */
    unsigned char   ar_sha[ETHER_ADDR_LEN];   /* sender hardware address      */
    uint32_t        ar_sip;             /* sender IP address            */
    unsigned char   ar_tha[ETHER_ADDR_LEN];   /* target hardware address      */
    uint32_t        ar_tip;             /* target IP address            */
} __attribute__ ((packed)) ;
typedef struct sr_arp_hdr sr_arp_hdr_t;



/* Add any additional helper methods here & don't forget to also declare
them in sr_router.h.






If you use any of these methods in sr_arpcache.c, you must also forward declare
them in sr_arpcache.h to avoid circular dependencies. Since sr_router
already imports sr_arpcache.h, sr_arpcache cannot import sr_router.h -KM */
