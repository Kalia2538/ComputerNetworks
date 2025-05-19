# README for Assignment 2: Router

Name: Kalia Brown

JHED: kbrow232

---


**DESCRIBE YOUR CODE AND DESIGN DECISIONS HERE**

This will be worth 10% of the assignment grade.

Assignment 2 was very complex in regards to the implementation, as well as testing my router. The files that I modified were:
1. router.c
- In this file, I implemented the following methods: sr_handlepacket(), create_arp_reply(), and find_rt_dest(). 
- find_rt_dest() is a helper method that I created in order to find the longest prefix match. I chose to make this a helper method for code readability. 
- create_arp_reply() is another helper function that I wrote, to create an arp reply packet, based on the given parameters.
- The majority of the implementation done in this file was done in the sr_handlepacket() function. This function took care of the packet handling functionality of the router. I separated the code by using a switch block, to determine if the recieved packet was an IP packet or an ARP packet. If we got an ARP packet, I determined if it was a reply or a request. If it was a request, and the router had the info, I would send back a reply. If we didnt have the info, the router didn't have the info, it would drop the packet. If we were given an ARP reply, we would add the information to our cache. If we recieved an IP packet, and it was addressed to our router and also was an icmp packet, we would verify its checksums and send back an echo reply. If it was not an icmp packet addressed to us, we send back and icmp type 3. If it was not addressed to us at all, we would then need to forward the packet, using the information in our routing table.
2. sr_arpcache.c
The functions that I implemented in here are: sr_arpcache_sweepreqs() and handle_arpreq().
- the sweep requests funciton was very easy to write, by calling the handle_arpreq() function.
- the handle_arpreq() function allows us to check if the request has expired and update our cache with requests.
3. Other:
- I added print statements to other files for debugging purposes.

_______________________________
This assignment was very dificult for me because I spent a lot of time writing code, that took a very long time to test. I think one of my major issues is that I could've made more helper functions in order to speed up my debugging process (ex. if i change the creation of one ICMP pkt, i have to change it everywhere if I don't have a helper function for it).
In addition, I also took a lot of time to make sure that I had the proper functionality for each of the types of packets recieved.
Lastly, it was very difficut to figure out what exactly might be wrong with my formatting of packets, since I had to print out the headers, compare values, and ensure the variable types were all compatible.
Overall, this assignment has helped me gain an in depth understanding of the inner workings of routers!
