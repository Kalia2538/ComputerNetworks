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
#define MIN_REQ_LEN 4



int proxy(char *proxy_port) {
  struct addrinfo hints;
  struct addrinfo *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  
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
      exit(1);
    }
    
    int b;
    if ((b = bind(sockfd, p->ai_addr, p->ai_addrlen)) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }
    break;
  }
  freeaddrinfo(servinfo);

  // listening for client connections
  int l = -91;
  if ((l = listen(sockfd, 10) == -1)) { // QUESTION: what should our queue length be?
    perror("listen");
    exit(1);
  }
  

  socklen_t sin_size;
  int new_fd; // fd for accepted client
  while(1) {    
    sin_size = sizeof(their_addr);    
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    
    if (new_fd == -1) {
      continue;
    }

    if (fork() == 0)
    {
      close(sockfd);
      printf("client connected to proxy\n");

      struct ParsedRequest* parsedReq = ParsedRequest_create();
      char buff[MAX_REQ_LEN];
      int val;
      int recd;
      char wholeReq[MAX_REQ_LEN] = ""; // double check this formatting
      int total_bytes = 0;

      int completedMessage = 0;
      int found = 0;
      // run until we recieve a message
      while (total_bytes < MAX_REQ_LEN) { // continuously recieve data from client --- then check for \r\n\r\n
        recd = recv(new_fd, buff+total_bytes, MAX_REQ_LEN - total_bytes, 0);
        printf("this is buff: %s", buff);
        total_bytes += recd;
        if (strstr(buff, "\r\n")) {
          printf("found ending!\n");
          break;
        }
      }

      memcpy(buff+recd, "\r\n", 2);
      val = ParsedRequest_parse(parsedReq, buff, total_bytes + 2);
      if(ParsedHeader_set(parsedReq, "Connection", "close") != 0) {
        printf("issue with connection header\n");
        return -1;
      }
      if (ParsedHeader_set(parsedReq, "Host", parsedReq->host) != 0) {
        printf("issue with host header\n");
        return -1;
      }

      printf("parsed the headers\n");
      
      char *def_port = "80";
      if (parsedReq->port == NULL) {
        parsedReq->port = def_port;
      }


      struct addrinfo hints2;
    struct addrinfo *servinfo2, *p2;
    struct sockaddr_storage their_addr2; // connector's address information

    memset(&hints2, 0, sizeof(hints2));
    hints2.ai_family = AF_UNSPEC;
    hints2.ai_socktype = SOCK_STREAM;
    int sockfd2;
    
    int rv2;
    if ((rv2 = getaddrinfo(parsedReq->host, parsedReq->port, &hints2, &servinfo2)) != 0) { //update thissss
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv2));
      return 1;
    }
    
    // establishing connection to the server
    printf("about to enter for loop to establish connection to the server\n");
    for(p2 = servinfo2; p2 != NULL; p2 = p2->ai_next) {
      if ((sockfd2 = socket(p2->ai_family, p2->ai_socktype,
        p2->ai_protocol)) == -1) {
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
          return 2;
      }
    }
    printf("connecting to server succeeded\n");

    int a = send(sockfd2, buff, total_bytes+4, 0); // send request to the server
    if (a == -1) {
      printf("error with sending request");
    }
    

    // wait for response
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
    int w = send(new_fd, servbuff, recd, 0);

      /* code */
    } else {
      close(new_fd);
    }
    // should have a fully parsed request by here

    // printf("got full request");

    // establish connection to the server (i think i get value from the parsedReq variable)

    // struct addrinfo hints2;
    // struct addrinfo *servinfo2, *p2;
    // struct sockaddr_storage their_addr2; // connector's address information

    // memset(&hints2, 0, sizeof(hints2));
    // hints2.ai_family = AF_UNSPEC;
    // hints2.ai_socktype = SOCK_STREAM;
    // int sockfd2;
    
    // int rv2;
    // if ((rv2 = getaddrinfo(parsedReq->host, parsedReq->port, &hints2, &servinfo2)) != 0) { //update thissss
    //   fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv2));
    //   return 1;
    // }
    
    // // establishing connection to the server
    // printf("about to enter for loop to establish connection to the server\n");
    // for(p2 = servinfo2; p2 != NULL; p2 = p2->ai_next) {
    //   if ((sockfd2 = socket(p2->ai_family, p2->ai_socktype,
    //     p2->ai_protocol)) == -1) {
    //     perror("client: socket");
    //     continue;
    //   }
  
    //   if (connect(sockfd2, p2->ai_addr, p2->ai_addrlen) == -1) {
    //     close(sockfd2);
    //       perror("client: connect");
    //         continue;
    //   }
  
    //   break;
  
    //   if (p2 == NULL) { // dc this line
    //       fprintf(stderr, "client: failed to connect\n");
    //       return 2;
    //   }
    // }
    // printf("connecting to server succeeded\n");

    // int a = send(sockfd2, buff, abc+4, 0); // send request to the server
    // if (a == -1) {
    //   printf("error with sending request");
    // }
    

    // // wait for response
    // int recd;
    // char servbuff[MAX_REQ_LEN] = "";
    // while (1) {
    //   recd = recv(sockfd2, servbuff, MAX_REQ_LEN, 0);
    //   if (recd > 0) {
    //     // printf(" this is buff: %s\n",servbuff);
    //     break;
    //   }
    // }
    // printf("recieved the response from the server!\n");


    // // close connection to server (not sure if necessary for this assignment)'
    // close(sockfd2);
    
    // // send response to the client
    // int w = send(new_fd, servbuff, recd, 0);

    

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
