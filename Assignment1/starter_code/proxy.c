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

/* TODO: proxy() == MAKE SURE TO CLOSE YOUR SOCKETS!!!!
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
  int proc_num = 0;
  
  // struct addrinfo hints;
  // struct addrinfo *servinfo, *p;
  // struct sockaddr_storage their_addr; // connector's address information
  struct sockaddr_in server;
  socklen_t x = sizeof(server);
  // printf("1\n");
  memset(&server, 0, sizeof(server));
  
  // printf("extra\n");
  // p->ai_family = AF_INET;
  // printf("a\n");
  // p->ai_socktype = SOCK_STREAM;
  // printf("b\n");
  // p->ai_flags = AI_PASSIVE;
  // printf("c\n");
  // p->ai_addr = INADDR_ANY;
  // printf("d\n");
  
  
  // int rv1;
  // if ((rv1 = getaddrinfo(NULL, proxy_port, &hints, &servinfo)) != 0) {
  //   fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv1));
  //   return 1;
  // }
  printf("2\n");
  int sockfd;
  int yes = 1;
  sockfd = socket(AF_INET, SOCK_STREAM,0);
  // establishing socket connection
  if (sockfd <0) {
      perror("server: socket");
  }
  printf("socket val is %d\n", sockfd);
  fflush(stdout);

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(atoi(proxy_port));
  if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
    // close(sockfd);
    perror("server: bind");
    fflush(stdout);
  }
  printf("bind");
  fflush(stdout);

  // listening for client connections
  int l = -91;
  if ((l = listen(sockfd, 10) == -1)) { // QUESTION: what should our queue length be?
    perror("listen");
    exit(1);
  }
  

  socklen_t sin_size;
  int new_fd; // fd for accepted client
  while(1) {    
    // sin_size = sizeof(their_addr); 
    printf("in the while loop\n");   
    new_fd = accept(sockfd, (struct sockaddr *)&server, &x);
    
    if (new_fd < 0) {
      perror("accept");
      continue;
    }
    printf("accept value is: %d", new_fd);

    int pid = fork();
    if (pid == 0) { // child process
      printf("in the child process\n");
      struct ParsedRequest* parsedReq = ParsedRequest_create();
      char buff[MAX_REQ_LEN];
      int val = -91;
      int abc = -1;
      char wholeReq[MAX_REQ_LEN] = ""; // double check this formatting
      int total_bytes = 0;

      int completedMessage = 0;
      int found = 0;

      // run until we recieve a message
      while (total_bytes < MAX_REQ_LEN) { // continuously recieve data from client --- then check for \r\n\r\n
        abc = recv(new_fd, buff+total_bytes, MAX_REQ_LEN - total_bytes, 0);
        printf("curr buff = %s", buff);
        // printf("recieved from the client %s\n", buff);
        total_bytes += abc;
        if (strstr(buff, "\r\n\r\n")) {
          // printf("found end of message\n");
          break;
          // found end of message
        }
        // if (abc > 0) {
        //   // printf(" this is buff: %s\0\n",buff);
        //   break;
        // }
      }
      continue;
      // add ending
      // strcat(buff, "\r\n\r\n"); /// might be better to use memcopy here
      memcpy(buff+abc, "\r\n", 2);
      val = ParsedRequest_parse(parsedReq, buff, abc + 2);
      // change the header
      if(ParsedHeader_set(parsedReq, "Connection", "close") != 0) {
        printf("issue with connection header\n");
        return -1;
      }
      if (ParsedHeader_set(parsedReq, "Host", parsedReq->host) != 0) {
        printf("issue with host header\n");
        return -1;
      }
      ParsedHeader_set(parsedReq, "Connection", "close");
      ParsedHeader_set(parsedReq, "Host", parsedReq->host);
      // printf("updated the headers\n");

      
      // printf("parsed the request\n");
      // printf("this is the method %s\n",parsedReq->method);
      // printf("this is the protocol %s\n",parsedReq->protocol);
      // printf("this is the host %s\n", parsedReq->host);
      // printf("this is the port_num %s\n", parsedReq->port);
      // printf("this is the path %s\n",parsedReq->path);
      // printf("this is the version %s\n",parsedReq->version);
      // printf("this is the buf %s\n",parsedReq->buf);
      char *def_port = "80";
      if (parsedReq->port == NULL) {
        parsedReq->port = def_port;
      }
      
      // size_t *temp = 0;
      // int randomc = ParsedRequest_printRequestLine(parsedReq, temp1, MAX_REQ_LEN, temp)
      if (val == -1) {
        printf("bad request");
        // return -1;
        // TODO: DO SOME KIND OF ERROR STUFF
      } else {
      }
      
      // should have a fully parsed request by here

      // printf("got full request");

      // establish connection to the server (i think i get value from the parsedReq variable)

      struct addrinfo hints2;
      struct addrinfo *servinfo2, *p2;
      struct sockaddr_storage their_addr2; // connector's address information

      memset(&hints2, 0, sizeof(hints2));
      hints2.ai_family = AF_UNSPEC;
      hints2.ai_socktype = SOCK_STREAM;
      int sockfd2;
      
      // int rv2;
      // if ((rv2 = getaddrinfo(parsedReq->host, parsedReq->port, &hints2, &servinfo2)) != 0) { //update thissss
      //   fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv2));
      //   return 1;
      // }
      // printf("rv2\n");
      
      // establishing connection to the server

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
      
  
      // printf("connected with the server\n");
      
      char unBuff[MAX_REQ_LEN];
      size_t unBuff_len =ParsedRequest_totalLen(parsedReq);
      ParsedRequest_unparse(parsedReq, unBuff, unBuff_len);
      // printf("successful unparse\n");
      ParsedRequest_destroy(parsedReq);
      int a = send(sockfd2, unBuff, unBuff_len, 0); // send request to the server
      if (a == -1) {
        printf("error with sending request");
      }
      

      // wait for response // just recieve everything continuously ... look at last assignment
      int recd;
      char servbuff[MAX_REQ_LEN] = "";
      while (1) { // continuously recieve data and search for buff (w/i max length)
        recd = recv(sockfd2, servbuff, MAX_REQ_LEN, 0);
        if (recd <= 0) {
          // printf(" this is buff: %s\n",servbuff);
          break;
        }
        send(new_fd, servbuff, recd, 0);
      }

      // printf("recieved from server\n");


      // close connection to server (not sure if necessary for this assignment)'
      close(sockfd2);
      close(sockfd2);
      
      // send response to the client
      // int w = send(new_fd, servbuff, recd, 0);   
      // printf("sent to the client :)\n");  
      exit(pid);
    
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
