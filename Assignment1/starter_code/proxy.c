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
  struct sockaddr_in server;
  int sockfd;

  socklen_t x = sizeof(server);
  memset(&server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(atoi(proxy_port));

  // creating the socket
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("proxy: socket not created\n");
    printf("socket failed with error: %s\n", strerror(errno));
    return -1;
  }
  printf("sockfd is: %d\n", sockfd);

  // binding address to the socket
  int bind_val;
  if ((bind_val = bind(sockfd, (struct sockaddr *)&server, sizeof(server)))< 0) {
    perror("proxy: bind unsuccesful\n");
    printf("Bind failed with error: %s\n", strerror(errno));
    close(sockfd);
    return -1;
  }
  printf("bind successful with return value %d\n",bind_val);

  // listening for client connections
  int listen_val;
  if (((listen_val = listen(sockfd, 10)) < 0)) { /* or == -1*/
    perror("proxy: listen unsuccessful");
    printf("listen failed with error: %s\n", strerror(errno));
    close(sockfd);
    return -1;
  }
  printf("listen successful with value %d\n", listen_val);
  

  int new_fd; // fd for accepted client
  while(1) {    
    if((new_fd = accept(sockfd, (struct sockaddr *)&server, &x)) < 0) {
      perror("proxy/client: accept failed");
      printf("accept failed with error: %s\n", strerror(errno));
      close(sockfd);
      return -1;
    }
    printf("accept successful with new_fd = %d\n", new_fd);
    
    int pid = fork();
    if (pid == 0) { // child process
      printf("in child process\n");
      struct ParsedRequest* parsedReq = ParsedRequest_create();
      char buff[MAX_REQ_LEN];
      
      int recv_bytes = 0;
      int total_bytes = 0;
      int found = 0;
      // run until we recieve a message
      while (total_bytes < MAX_REQ_LEN) { // continuously recieve data from client --- then check for \r\n\r\n
        recv_bytes = recv(new_fd, buff+total_bytes, MAX_REQ_LEN - total_bytes, 0);
        printf("this is buff: %s", buff);
        total_bytes += recv_bytes;
        if (strstr(buff, "\r\n")) {
          printf("found ending!\n");
          found = 1;
          break;
        }
      }
      if (found == 0) {
        perror("request is too large\n");
        ParsedRequest_destroy(parsedReq);
        close(sockfd);
        return -1;
      }

      // continue;
      printf("message completed%s\n", buff);
      // add the ending
      memcpy(buff+total_bytes, "\r\n", 2);

      if ((ParsedRequest_parse(parsedReq, buff, total_bytes + 2)) < 0) {
        perror("unsuccessful parse\n");
        ParsedRequest_destroy(parsedReq);
        close(sockfd);
        return -1;
      }

      if(ParsedHeader_set(parsedReq, "Connection", "close") != 0) {
        printf("issue with connection header\n");
        ParsedRequest_destroy(parsedReq);
        close(sockfd);
        return -1;
      }
      if (ParsedHeader_set(parsedReq, "Host", parsedReq->host) != 0) {
        printf("issue with host header\n");
        ParsedRequest_destroy(parsedReq);
        close(sockfd);
        return -1;
      }

      printf("parsed the headers\n");
      
      // checking for default port number
      char *def_port = "80";
      if (parsedReq->port == NULL) {
        parsedReq->port = def_port;
      }
          

      struct hostent *h = gethostbyname(parsedReq->host); // error check here
      if (h == NULL) {
        herror("gethostbyname - error");
        ParsedRequest_destroy(parsedReq);
        close(sockfd);
        return -1;  // Return error if resolution fails
      }

      struct sockaddr_in server_ad;
      memset(&server_ad, 0, sizeof(server_ad));
      server_ad.sin_family = AF_INET;
      memcpy(&server_ad.sin_addr.s_addr, h->h_addr, h->h_length);

      int sockfd2;
      if ((sockfd2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("proxy/server: socket not created\n");
        printf("socket failed with error: %s\n", strerror(errno));
        ParsedRequest_destroy(parsedReq);
        close(sockfd);
        return -1;
      }
      printf("this is sockfd2: %d\n", sockfd2);
      
      
      // server_ad.sin_addr.s_addr = ;  // dc this for errors
      server_ad.sin_port = htons(atoi(parsedReq->port));
      
      int num;
      if ((num = connect(sockfd2, (struct sockaddr *) &server_ad, sizeof(server_ad))) < 0) {
        perror("proxy/server: connection unsuccessful\n");
        printf("connect failed with error: %s\n", strerror(errno));
        ParsedRequest_destroy(parsedReq);
        close(sockfd);
        close(sockfd2);
        return -1;
      }
      printf("connect succeeded\n");
      
      char unBuff[MAX_REQ_LEN];
      size_t unBuff_len = ParsedRequest_totalLen(parsedReq);
      int unp = ParsedRequest_unparse(parsedReq, unBuff, unBuff_len);
      if (unp < 0) {
        perror("unsuccessful unparse\n");
        ParsedRequest_destroy(parsedReq);
        close(sockfd);
        close(sockfd2);

      }
      printf("successful unparse\n");
      ParsedRequest_destroy(parsedReq);

      // sending request to the server
      int num_bytes;
      if ((num_bytes = (send(sockfd2, unBuff, unBuff_len, 0))) < 0) {
        printf("error with sending request");
        close(sockfd);
        close(sockfd2);
        return -1;
      }

      // wait for response // just recieve everything continuously ... look at last assignment
      int recd;
      char servbuff[MAX_REQ_LEN] = "";
      while (1) { // continuously recieve data and search for buff (w/i max length)
        recd = recv(sockfd2, servbuff, MAX_REQ_LEN, 0);
        if (recd < 0) {
          perror("server/proxy: error receiving data\n");
          printf("recv failed with error: %s\n", strerror(errno));
          close(sockfd);
          close(sockfd2);
          return -1;
        } else if (recd == 0) {
          break; // no more data to send
        }
        send(new_fd, servbuff, recd, 0);
      }

      close(sockfd);
      close(sockfd2); 
      exit(pid);
    
    }   
    close(sockfd);

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
