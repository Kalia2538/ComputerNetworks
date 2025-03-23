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
  struct sr_ethernet_hdr *eth_hdr = (struct sr_ethernet_hdr *) packet;

  switch (result) {
    case 1: // arp
      // do stuff
      break;
    case 9: // ip
      // do stuff
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
  struct  sr_ethernet_hdr *hdr = (struct sr_ethernet_hdr *) packet;
  if (hdr->ether_type == ethertype_arp) { // arp packet
    return 1;
  } else if (hdr->ether_type == ethertype_ip) { // ip packet
    return 9;
  } else {
    return 0; // none of the above
  }
}



/* Add any additional helper methods here & don't forget to also declare
them in sr_router.h.






If you use any of these methods in sr_arpcache.c, you must also forward declare
them in sr_arpcache.h to avoid circular dependencies. Since sr_router
already imports sr_arpcache.h, sr_arpcache cannot import sr_router.h -KM */
