#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <netinet/tcp.h>

/* TODO: proxy()
 * Establish a socket connection to listen for incoming connections.
 * Accept each client request in a new process.
 * Parse header of request and get requested URL.
 * Get data from requested remote server.
 * Send data to the client
 * Return 0 on success, non-zero on failure
*/

#define DEFAULT_NHDRS 8
#define MAX_REQ_LEN 65535
#define MIN_REQ_LEN 4#define DEFAULT_NHDRS 8
#define MAX_REQ_LEN 65535
#define MIN_REQ_LEN 4



int proxy(char *proxy_port) {
  struct addrinfo hints;
  struct addrinfo *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  int rv;
  if ((rv = getaddrinfo("127.0.0.1", proxy_port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  int sockfd;
  int yes = 1;

  // establishing socket connection
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
      p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  // listening for client connections
  if (listen(sockfd, 10) == -1) { // QUESTION: what should our queue length be?
    perror("listen");
    exit(1);
  }

  socklen_t sin_size;
  int new_fd;
  while(1) {  // main accept() loop
    sin_size = sizeof(their_addr);
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    // start the forking process here
    // for now, we will only do one conection at a time

    struct ParsedRequest* parsedReq = ParsedRequest_create();
    char buff[MAX_REQ_LEN];
    int buffLen;
    int val; 
    
    while ((val = ParsedRequest_parse(parsedReq, buff, buffLen)) >0 ) {
      continue; // double check this
    }

    if (val == -1) {
      // bad request
      return -1;
    }

    // should have a fully parsed request by here

    

  }
  return 0;
}





int main(int argc, char * argv[]) {
  char *proxy_port;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./proxy <port>\n");
    exit(EXIT_FAILURE);
  }

  proxy_port = argv[1];
  return proxy(proxy_port);
}
