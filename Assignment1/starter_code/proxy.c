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

/* TODO: proxy() //
 * Establish a socket connection to listen for incoming connections.
 * Accept each client request in a new process.
 * Parse header of request and get requested URL.
 * Get data from requested remote server.
 * Send data to the client
 * Return 0 on success, non-zero on failure
*/

#define DEFAULT_NHDRS 8
#define MAX_REQ_LEN 65535
#define MIN_REQ_LEN 4

void client_helper(int client_fd) {
  struct ParsedRequest* parsedReq = ParsedRequest_create();
  char buff[MAX_REQ_LEN];
  int val;

  int recd;

  // run until we recieve a message
  while (1) { // possibly change to reading and searching for the ending
    recd = recv(client_fd, buff, MAX_REQ_LEN, 0);
    if (recd > 0) {
      // printf(" this is buff: %s\0\n",buff);
      break;
    }
  }
  
  // add ending
  strcat(buff, "\r\n\r\n"); // go back to reading bytes and searching for ending
  val = ParsedRequest_parse(parsedReq, buff, recd+4);
  if (val < 0) {
    char message[] = "HTTP/1.0 400 Bad Request\r\n\r\n";
    send(client_fd, message, strlen(message), 0);
    ParsedRequest_destroy(parsedReq);
    exit(1);
    // destroy request and exit
  }
  if(ParsedHeader_set(parsedReq, "Connection", "close") != 0) {
    char message[] = "HTTP/1.0 400 Bad Request\r\n\r\n";
    send(client_fd, message, strlen(message), 0);
    ParsedRequest_destroy(parsedReq);
    exit(1);
  }
  if (ParsedHeader_set(parsedReq, "Host", parsedReq->host) != 0) {
    char message[] = "HTTP/1.0 400 Bad Request\r\n\r\n";
    send(client_fd, message, strlen(message), 0);
    ParsedRequest_destroy(parsedReq);
    exit(1);
  }

  char *def_port = "80";
  if (parsedReq->port == NULL) {
    parsedReq->port = def_port;
  }
  
  // size_t *temp = 0;
  // int randomc = ParsedRequest_printRequestLine(parsedReq, temp1, MAX_REQ_LEN, temp)
  printf("successfully parsed the message!!!");

  
  // should have a fully parsed request by here

  // printf("got full request");

  // establish connection to the server (i think i get value from the parsedReq variable)

  struct addrinfo hints2;
  struct addrinfo *servinfo2, *p2;

  memset(&hints2, 0, sizeof(hints2));
  hints2.ai_family = AF_UNSPEC;
  hints2.ai_socktype = SOCK_STREAM;
  int sockfd2;
  
  int rv2;
  if ((rv2 = getaddrinfo(parsedReq->host, parsedReq->port, &hints2, &servinfo2)) != 0) { //update thissss
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv2));
    return;
  }
  
  // establishing connection to the server
  printf("about to enter for loop to establish connection to the server\n");
  for(p2 = servinfo2; p2 != NULL; p2 = p2->ai_next) {
    if ((sockfd2 = socket(p2->ai_family, p2->ai_socktype, p2->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd2, p2->ai_addr, p2->ai_addrlen) == -1) {
      close(sockfd2);
      perror("client: connect");
      continue;
    }

    break;

    if (p2 == NULL) { // dc this line
        fprintf(stderr, "client: failed to connect\n");
        return;
    }
  }
  printf("connecting to server succeeded\n");

  int a = send(sockfd2, buff, recd+4, 0); // send request to the server
  if (a == -1) {
    printf("error with sending request");
    char message[] = "HTTP/1.0 500 server error\r\n\r\n";
    send(client_fd, message, strlen(message), 0);
  }
  

  // wait for response
  recd = 0;
  char servbuff[MAX_REQ_LEN] = "";
  while (1) {
    recd = recv(sockfd2, servbuff, MAX_REQ_LEN, 0);
    if (recd > 0) {
      // printf(" this is buff: %s\n",servbuff);
      break;
    }
  }

  printf("recieved the response from the server!\n");


  // close connection to server (not sure if necessary for this assignment)'
  close(sockfd2);
    
  // send response to the client
  send(client_fd, servbuff, recd, 0);
}

int proxy(char *proxy_port) {
  struct addrinfo hints;
  struct addrinfo *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  
  
  int rv1;
  if ((rv1 = getaddrinfo(NULL, proxy_port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv1));
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

    int set;
    if ((set = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int))) == -1) {
      perror("setsockopt");
      return 1;
    }
    
    int b;
    if ((b = bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
      close(sockfd);
      perror("server: bind");
    }
    break;
  }
  freeaddrinfo(servinfo);

  if (p == NULL) {
    return 1;
  }

  // listening for client connections
  int l = -91;
  if ((l = listen(sockfd, 10) == -1)) { // QUESTION: what should our queue length be?
    perror("listen");
    return 1;
  }
  

  socklen_t sin_size;
  int new_fd; // fd for accepted client
  while(1) {    
    sin_size = sizeof(their_addr);    
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    
    if (new_fd == -1) {
      continue;
    }

    if (!fork()){
    
      close(sockfd);
      client_helper(new_fd);
      exit(1);
    }  else {
      close(new_fd);
    }
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
